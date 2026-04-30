#include "sched/sched.h"
#include "sched_internal.h"

void arch_post_switch(void) {
  uint32_t core = sched_clamp_core(hal_cpu_get_id());
  hal_cpu_enable_interrupts();
}

void sched_reschedule(void) {
  uint32_t core = sched_clamp_core(hal_cpu_get_id());
  sched_reap_terminated_threads();
  sched_process_pending_ai_suggestions();

  hal_cpu_disable_interrupts(); // Fast path local lockless

  sched_rq_t *rq = &g_cpu_locals[core].runqueue;

  // Empty remote scheduler command inbox
  if (rq->resched_pending != 0U || !list_empty(&rq->pending_inbox)) {
      spin_lock(&rq->lock);
      rq->resched_pending = 0U; // Clear flag since we are draining now
      list_head_t *curr = rq->pending_inbox.next;
      uint32_t drained = 0;
      bh_thread_t *highest_prio_arrived = NULL;

      while (curr != &rq->pending_inbox) {
          sched_remote_cmd_t *cmd = (sched_remote_cmd_t *)(void *)((char *)curr - offsetof(sched_remote_cmd_t, list));
          curr = curr->next;

          list_del(&cmd->list);
          list_init(&cmd->list);

          bh_thread_t* thread = sched_find_thread_by_id(cmd->thread_id);
          if (!thread) {
              continue;
          }

          // Validation: Check generation
          if (thread->sched_generation != cmd->expected_thread_generation) {
              cmd->state = SCHED_REMOTE_CMD_FAILED;
              continue;
          }

          // Invariant: Verify destination CPU matches expected owner
          if (thread->owner_cpu != core &&
              thread->owner_state != THREAD_OWNER_REMOTE_PENDING &&
              thread->owner_state != THREAD_OWNER_NONE) {
              cmd->state = SCHED_REMOTE_CMD_FAILED;
              continue;
          }

          thread_slot_t *slot = sched_find_thread_slot_by_tid_local(&g_cpu_locals[thread->home_core_id].runqueue, thread->thread_id);
          if (!slot) {
              cmd->state = SCHED_REMOTE_CMD_FAILED;
              continue;
          }

          if (cmd->type == SCHED_REMOTE_WAKE) {
              if (cmd->priority <= SCHED_MAX_PRIORITY && cmd->priority > thread->priority) {
                  thread->priority = cmd->priority;
              }
              if (thread->state != THREAD_STATE_SLEEPING && thread->state != THREAD_STATE_BLOCKED) {
                  continue;
              }
              thread->wake_deadline_ms = 0U;
              if (slot->is_sleeping != 0U) {
                sched_sleep_dequeue(slot);
              }
              if (slot->is_blocked != 0U) {
                sched_block_dequeue(slot);
              }
          } else if (cmd->type == SCHED_REMOTE_MIGRATE) {
              // Remote migration Phase 1: Old owner dequeue
              if (slot->is_on_runqueue != 0U) {
                  sched_invariant_on_dequeue(thread);
                  if (g_policy == SCHED_POLICY_CLOUD_FAIR) {
                      sched_cfs_dequeue(rq, thread);
                  } else {
                      list_del(&slot->run_node);
                      list_init(&slot->run_node);
                      sched_ready_bitmap_clear_if_empty(rq, thread->priority);
                  }
                  slot->is_on_runqueue = 0U;
                  if (rq->runnable_count > 0U) {
                      rq->runnable_count--;
                  }
              }
              thread->owner_state = THREAD_OWNER_REMOTE_PENDING;
              thread->migration_state = SCHED_MIGRATION_DEQUEUED;

              // Transition to Phase 2: Remote enqueue request to target CPU
              uint32_t target_cpu = thread->migration_target_cpu;
              sched_rq_t *target_rq = &g_cpu_locals[target_cpu].runqueue;

              // We reuse the current cmd slot or assume a new one is available in Phase 2
              // For simplicity and safety, we trigger Phase 2 enqueue here
              spin_lock(&target_rq->lock);
              cmd->type = SCHED_REMOTE_ENQUEUE;
              cmd->source_cpu = core;
              cmd->target_cpu = target_cpu;
              cmd->state = SCHED_REMOTE_CMD_PENDING;

              list_add_tail(&cmd->list, &target_rq->pending_inbox);
              if (target_rq->resched_pending == 0) {
                  target_rq->resched_pending = 1;
                  spin_unlock(&target_rq->lock);
                  hal_send_ipi_payload(1U << target_cpu, MK_MSG_THREAD_ENQUEUE_REQ);
              } else {
                  spin_unlock(&target_rq->lock);
              }
              thread->migration_state = SCHED_MIGRATION_ENQUEUE_REQUESTED;
              continue; // Command is now pending on target_cpu
          } else if (cmd->type == SCHED_REMOTE_ENQUEUE) {
              if (thread->migration_state == SCHED_MIGRATION_COMMITTED) {
                  // Already handled
                  cmd->state = SCHED_REMOTE_CMD_ACKED;
                  continue;
              }

              if (thread->owner_state == THREAD_OWNER_REMOTE_PENDING) {
                  // Validate admissibility on target core
                  if (!sched_is_core_admissible(thread, core)) {
                      // Rollback attempt to old owner
                      cmd->state = SCHED_REMOTE_CMD_FAILED;
                      uint32_t old_owner = cmd->source_cpu;
                      sched_rq_t *old_rq = &g_cpu_locals[old_owner].runqueue;

                      spin_lock(&old_rq->lock);
                      cmd->type = SCHED_REMOTE_ENQUEUE; // Rollback enqueue
                      cmd->target_cpu = old_owner;
                      cmd->source_cpu = core;
                      cmd->state = SCHED_REMOTE_CMD_PENDING;
                      list_add_tail(&cmd->list, &old_rq->pending_inbox);

                      if (old_rq->resched_pending == 0) {
                          old_rq->resched_pending = 1;
                          spin_unlock(&old_rq->lock);
                          hal_send_ipi_payload(1U << old_owner, MK_MSG_THREAD_ENQUEUE_REQ);
                      } else {
                          spin_unlock(&old_rq->lock);
                      }
                      thread->migration_state = SCHED_MIGRATION_ROLLBACK_REQUESTED;
                      continue;
                  }

                  thread->owner_state = THREAD_OWNER_NONE;
                  thread->owner_cpu = core;
                  thread->bound_core_id = core;
                  thread->migration_state = SCHED_MIGRATION_COMMITTED;
                  cmd->state = SCHED_REMOTE_CMD_ACKED;
              } else if (thread->migration_state == SCHED_MIGRATION_ROLLBACK_REQUESTED) {
                  // Successful rollback
                  thread->owner_state = THREAD_OWNER_NONE;
                  thread->owner_cpu = core;
                  // bound_core_id didn't change yet or we restore it
                  thread->bound_core_id = core;
                  thread->migration_state = SCHED_MIGRATION_NONE; // Reset
                  cmd->state = SCHED_REMOTE_CMD_ACKED;
              } else if (thread->owner_state == THREAD_OWNER_REMOTE_PENDING &&
                         thread->migration_state == SCHED_MIGRATION_ROLLBACK_REQUESTED) {
                  // Rollback failed as well -> Quarantine
                  sched_quarantine_thread(thread, THREAD_FAULT_MIGRATION_ROLLBACK_FAILED);
                  thread->migration_state = SCHED_MIGRATION_FAILED;
                  cmd->state = SCHED_REMOTE_CMD_FAILED;
                  continue;
              } else if (thread->migration_state == SCHED_MIGRATION_ENQUEUE_REQUESTED && thread->bound_core_id == core) {
                  // This handles local re-enqueue or redundant enqueue
                  thread->migration_state = SCHED_MIGRATION_NONE;
                  cmd->state = SCHED_REMOTE_CMD_ACKED;
              }
          } else if (cmd->type == SCHED_REMOTE_DEQUEUE) {
              if (slot->is_on_runqueue != 0U) {
                  sched_invariant_on_dequeue(thread);
                  if (g_policy == SCHED_POLICY_CLOUD_FAIR) {
                      sched_cfs_dequeue(rq, thread);
                  } else {
                      list_del(&slot->run_node);
                      list_init(&slot->run_node);
                      sched_ready_bitmap_clear_if_empty(rq, thread->priority);
                  }
                  slot->is_on_runqueue = 0U;
                  if (rq->runnable_count > 0U) {
                      rq->runnable_count--;
                  }
              }
              continue; // DEQUEUE is complete
          } else if (cmd->type == SCHED_REMOTE_QUARANTINE) {
              sched_quarantine_thread(thread, 0xBAD);
              if (slot->is_on_runqueue != 0U) {
                  sched_invariant_on_dequeue(thread);
                  if (g_policy == SCHED_POLICY_CLOUD_FAIR) {
                      sched_cfs_dequeue(rq, thread);
                  } else {
                      list_del(&slot->run_node);
                      list_init(&slot->run_node);
                      sched_ready_bitmap_clear_if_empty(rq, thread->priority);
                  }
                  slot->is_on_runqueue = 0U;
                  if (rq->runnable_count > 0U) {
                      rq->runnable_count--;
                  }
              }
              continue;
          }

          if (thread->state == THREAD_STATE_QUARANTINED || thread->owner_state == THREAD_OWNER_QUARANTINED) {
              if (thread->migration_state == SCHED_MIGRATION_ROLLBACK_REQUESTED) {
                  thread->migration_state = SCHED_MIGRATION_FAILED;
                  thread->pending_fault = THREAD_FAULT_MIGRATION_ROLLBACK_FAILED;
              }
              continue;
          }

          if (cmd->type == SCHED_REMOTE_WAKE) {
              thread->state = THREAD_STATE_READY;
          }

          if (thread->migration_state == SCHED_MIGRATION_COMMITTED) {
              thread->migration_state = SCHED_MIGRATION_NONE;
          }
          sched_invariant_on_enqueue(thread, core);

          if (g_policy == SCHED_POLICY_CLOUD_FAIR) {
            sched_cfs_enqueue(rq, thread);
          } else if (g_policy == SCHED_POLICY_EDF) {
            if (thread->rt_attr.period_ms > 0 && thread->rt_attr.deadline_ms > 0) {
                if (thread->absolute_deadline_ms == 0) {
                    thread->absolute_deadline_ms = g_cpu_locals[core].runqueue.total_ticks + thread->rt_attr.deadline_ms;
                }
            }
            sched_edf_enqueue(rq, thread);
          } else {
            list_add(&slot->run_node, &rq->ready_queue[thread->priority]);
            sched_ready_bitmap_set(rq, thread->priority);
          }

          if (!highest_prio_arrived || thread->priority > highest_prio_arrived->priority) {
              highest_prio_arrived = thread;
          }

          slot->is_on_runqueue = 1U;
          rq->runnable_count++;
          drained++;
      }
      if (drained > 0) {
          rq->inbox_drains++;
          if (rq->current_thread && highest_prio_arrived &&
              highest_prio_arrived->priority > rq->current_thread->priority) {
              rq->remote_preemptions++;
          }
      }
      spin_unlock(&rq->lock);
  }

  if (g_cpu_locals[core].runqueue.throttled != 0U && g_cpu_locals[core].runqueue.idle_thread) {
    sched_switch_to(g_cpu_locals[core].runqueue.idle_thread, core);
    return;
  }

  bh_thread_t *next = sched_pick_next_ready(core);
  sched_switch_to(next, core);
}

void sched_on_timer_tick(void) {
  g_cpu_locals[sched_clamp_core(hal_cpu_get_id())].runqueue.total_ticks++;


  uint32_t core = sched_clamp_core(hal_cpu_get_id());

  ipc_async_check_timeouts(g_cpu_locals[core].runqueue.total_ticks);

  list_head_t *sleep_head = &g_cpu_locals[core].runqueue.sleeping_list;
  list_head_t *curr = sleep_head->next;
  while (curr != sleep_head) {
    thread_slot_t *slot = (thread_slot_t *)(void *)((char *)curr - offsetof(thread_slot_t, wait_node));
    curr = curr->next;
    if (slot->thread.state == THREAD_STATE_SLEEPING &&
        slot->thread.wake_deadline_ms <= g_cpu_locals[core].runqueue.total_ticks) {
      sched_wakeup(&slot->thread);
    }
  }

  list_head_t *block_head = &g_cpu_locals[core].runqueue.blocked_list;
  curr = block_head->next;
  while (curr != block_head) {
    thread_slot_t *slot = (thread_slot_t *)(void *)((char *)curr - offsetof(thread_slot_t, wait_node));
    curr = curr->next;
    if (slot->thread.state == THREAD_STATE_BLOCKED &&
        slot->thread.ipc_deadline_ticks > 0 &&
        slot->thread.ipc_deadline_ticks <= g_cpu_locals[core].runqueue.total_ticks) {
      slot->thread.ipc_wakeup_reason = -3; // IPC_ERR_WOULD_BLOCK or TIMEOUT
      slot->thread.ipc_deadline_ticks = 0;

      // Unlink it from wait queues handled by endpoint access so we can awaken it
      slot->thread.next_waiter = NULL;
      sched_wakeup(&slot->thread);
    }
  }

  sched_process_pending_ai_suggestions();
  sched_reap_terminated_threads();

  if ((g_cpu_locals[core].runqueue.total_ticks % 16U) == 0U && core == 0U) {
    sched_balance_once();
  }

  sched_rq_t* rq = &g_cpu_locals[core].runqueue;
  bh_thread_t *current = rq->current_thread;
  if (!current) {
    sched_reschedule();
    return;
  }

  current->cpu_time_consumed++;

  if (g_policy == SCHED_POLICY_CLOUD_FAIR && current != rq->idle_thread) {
    sched_cfs_update_vruntime(rq, current, 1);
  }

  sched_update_telemetry(current);

  if (g_policy == SCHED_POLICY_EDF && current != rq->idle_thread) {
      if (current->cpu_time_consumed >= current->rt_attr.wcet_ms) {
          // Task exhausted budget for this period, wait for next period
          current->absolute_deadline_ms += current->rt_attr.period_ms;
          current->cpu_time_consumed = 0U;

          thread_slot_t *slot = sched_find_thread_slot_by_tid_local(rq, current->thread_id);
          if (slot) {
              // Suspend the thread until the start of the next period
              current->wake_deadline_ms = current->absolute_deadline_ms - current->rt_attr.deadline_ms;
              current->state = THREAD_STATE_SLEEPING;
              sched_sleep_enqueue(slot, core);
              rq->current_thread = NULL;
          }
          sched_reschedule();
          return;
      }

      bh_thread_t *next = sched_edf_pick_next(rq);
      if (next && next->absolute_deadline_ms < current->absolute_deadline_ms) {
          sched_reschedule();
          return;
      }
  } else {
      if (current->cpu_time_consumed >= current->time_slice_ms) {
        current->cpu_time_consumed = 0U;
        sched_reschedule();
        return;
      }

      if (g_policy == SCHED_POLICY_CLOUD_FAIR) {
          bh_thread_t *next = sched_cfs_pick_next(rq);
          if (next && next->vruntime < current->vruntime) {
              sched_reschedule();
              return;
          }
      } else {
          uint32_t higher_mask = (current->priority >= SCHED_MAX_PRIORITY)
                                     ? 0U
                                     : ((~0U) << (current->priority + 1U));
          if ((rq->ready_bitmap & higher_mask) != 0U) {
            sched_reschedule();
            return;
          }
      }
  }
}

