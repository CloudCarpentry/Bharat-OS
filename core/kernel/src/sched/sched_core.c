#include "sched/sched.h"
#include "sched_internal.h"
#include "panic.h"

void arch_post_switch(void) {
  uint32_t core = sched_clamp_core(hal_cpu_get_id());
  hal_cpu_enable_interrupts();
}

static void sched_remote_respond(const sched_remote_cmd_envelope_t *env, uint8_t kind, int32_t result) {
    sched_remote_completion_t comp;
    comp.handle = env->handle;
    comp.result = result;
    comp.responder_cpu = (uint16_t)sched_clamp_core(hal_cpu_get_id());
    comp.kind = kind;
    comp.reserved = 0;

    sched_rq_t *origin_rq = &g_cpu_locals[env->handle.origin_cpu].runqueue;
    sched_completion_ring_push(&origin_rq->remote.completion_ring, &comp);
}

void sched_reschedule(void) {
  uint32_t core = sched_clamp_core(hal_cpu_get_id());
  sched_remote_cmd_poll_timeouts();
  sched_reap_terminated_threads();
  sched_process_pending_ai_suggestions();

  hal_cpu_disable_interrupts(); // Fast path local lockless

  sched_rq_t *rq = &g_cpu_locals[core].runqueue;

  // Empty remote scheduler command inbox (MPSC Lock-Free)
  if (rq->remote.resched_pending != 0U || !sched_cmd_ring_empty(&rq->remote.cmd_ring)) {
      spin_lock(&rq->lock);
      rq->remote.resched_pending = 0U; // Clear flag since we are draining now
      uint32_t drained = 0;
      bh_thread_t *highest_prio_arrived = NULL;

      sched_remote_cmd_envelope_t envelope;
      while (sched_cmd_ring_pop(&rq->remote.cmd_ring, &envelope) == K_OK) {
          __atomic_fetch_add(&rq->remote.consumed, 1, __ATOMIC_RELAXED);

          if (envelope.type == SCHED_REMOTE_STEAL_REQ) {
              bh_thread_t *victim = sched_find_steal_candidate(core, envelope.target_cpu);
              if (victim) {
                  thread_slot_t *v_slot = sched_find_thread_slot_by_tid(victim->thread_id);
                  if (v_slot) {
                      if (v_slot->is_on_runqueue != 0U) {
                          sched_invariant_on_dequeue(victim);
                          if (g_policy == SCHED_POLICY_CLOUD_FAIR) {
                              sched_cfs_dequeue(rq, victim);
                          } else {
                              list_del(&v_slot->run_node);
                              list_init(&v_slot->run_node);
                              sched_ready_bitmap_clear_if_empty(rq, victim->priority);
                          }
                          v_slot->is_on_runqueue = 0U;
                          if (rq->runnable_count > 0U) {
                              rq->runnable_count--;
                          }
                      }
                      uint32_t epoch = __atomic_fetch_add(&victim->migration_epoch, 1, __ATOMIC_SEQ_CST) + 1;
                      victim->owner_state = THREAD_OWNER_REMOTE_PENDING;
                      victim->migration_state = SCHED_MIGRATION_DEQUEUED;
                      victim->migration_target_cpu = envelope.target_cpu;

                      sched_remote_cmd_t *enq_cmd = sched_allocate_outbound_cmd();
                      if (enq_cmd) {
                          enq_cmd->type = SCHED_REMOTE_ENQUEUE;
                          enq_cmd->source_cpu = core;
                          enq_cmd->target_cpu = envelope.target_cpu;
                          enq_cmd->thread_id = victim->thread_id;
                          enq_cmd->expected_thread_generation = victim->sched_generation;
                          enq_cmd->migration_epoch = epoch;
                          enq_cmd->priority = victim->priority;
                          enq_cmd->state = SCHED_REMOTE_CMD_PENDING;

                          kstatus_t status = sched_remote_submit(envelope.target_cpu, enq_cmd);
                          if (status == K_OK) {
                              victim->migration_state = SCHED_MIGRATION_COMMIT_SENT;
                          } else {
                              sched_remote_cmd_release(enq_cmd);
                              victim->owner_state = THREAD_OWNER_NONE;
                              victim->migration_state = SCHED_MIGRATION_NONE;
                              sched_invariant_on_enqueue(victim, core);
                              if (g_policy == SCHED_POLICY_CLOUD_FAIR) {
                                  sched_cfs_enqueue(rq, victim);
                              } else {
                                  list_add(&v_slot->run_node, &rq->ready_queue[victim->priority]);
                                  sched_ready_bitmap_set(rq, victim->priority);
                              }
                              v_slot->is_on_runqueue = 1U;
                              rq->runnable_count++;
                          }
                      } else {
                          victim->owner_state = THREAD_OWNER_NONE;
                          victim->migration_state = SCHED_MIGRATION_NONE;
                          sched_invariant_on_enqueue(victim, core);
                          if (g_policy == SCHED_POLICY_CLOUD_FAIR) {
                              sched_cfs_enqueue(rq, victim);
                          } else {
                              list_add(&v_slot->run_node, &rq->ready_queue[victim->priority]);
                              sched_ready_bitmap_set(rq, victim->priority);
                          }
                          v_slot->is_on_runqueue = 1U;
                          rq->runnable_count++;
                      }
                  }
              }
              sched_remote_respond(&envelope, SCHED_COMPLETION_ACK, 0);
              continue;
          }

          bh_thread_t* thread = sched_find_thread_by_id(envelope.thread_id);
          if (!thread) {
              sched_remote_respond(&envelope, SCHED_COMPLETION_NACK, -1);
              continue;
          }

          // Validation: Check generation
          if (thread->sched_generation != envelope.expected_thread_generation) {
              sched_remote_respond(&envelope, SCHED_COMPLETION_NACK, -2);
              continue;
          }

          // Invariant: Verify destination CPU matches expected owner
          if (thread->owner_cpu != core &&
              thread->owner_state != THREAD_OWNER_REMOTE_PENDING &&
              thread->owner_state != THREAD_OWNER_NONE) {
              sched_remote_respond(&envelope, SCHED_COMPLETION_NACK, -3);
              continue;
          }

          thread_slot_t *slot = sched_find_thread_slot_by_tid(thread->thread_id);
          if (!slot) {
              sched_remote_respond(&envelope, SCHED_COMPLETION_NACK, -4);
              continue;
          }

          if (envelope.type == SCHED_REMOTE_WAKE) {
              if (envelope.priority <= SCHED_MAX_PRIORITY && envelope.priority > thread->priority) {
                  thread->priority = envelope.priority;
              }
              if (thread->state != THREAD_STATE_SLEEPING && thread->state != THREAD_STATE_BLOCKED) {
                  sched_remote_respond(&envelope, SCHED_COMPLETION_ACK, 0);
                  continue;
              }
              thread->wake_deadline_ms = 0U;
              if (slot->is_sleeping != 0U) {
                sched_sleep_dequeue(slot);
              }
              if (slot->is_blocked != 0U) {
                sched_block_dequeue(slot);
              }
          } else if (envelope.type == SCHED_REMOTE_MIGRATE || envelope.type == SCHED_REMOTE_MIGRATE_PREPARE) {
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
              __atomic_store_n(&thread->owner_state, THREAD_OWNER_REMOTE_PENDING, __ATOMIC_RELEASE);
              __atomic_store_n(&thread->migration_state, SCHED_MIGRATION_DEQUEUED, __ATOMIC_RELEASE);

              sched_remote_respond(&envelope, SCHED_COMPLETION_ACK, 0);
              continue;
          } else if (envelope.type == SCHED_REMOTE_ENQUEUE) {
              uint32_t mig_state = __atomic_load_n(&thread->migration_state, __ATOMIC_ACQUIRE);
              if (mig_state == SCHED_MIGRATION_COMMITTED) {
                  // Already handled
                  sched_remote_respond(&envelope, SCHED_COMPLETION_ACK, 0);
                  continue;
              }

              uint32_t o_state = __atomic_load_n(&thread->owner_state, __ATOMIC_ACQUIRE);
              if (o_state == THREAD_OWNER_REMOTE_PENDING) {
                  // Validate admissibility on target core
                  if (!sched_is_core_admissible(thread, core)) {
                      // Reject commit
                      sched_remote_respond(&envelope, SCHED_COMPLETION_NACK, -5);
                      continue;
                  }

                  __atomic_store_n(&thread->owner_cpu, core, __ATOMIC_RELEASE);
                  thread->bound_core_id = core;
                  __atomic_store_n(&thread->owner_state, THREAD_OWNER_NONE, __ATOMIC_RELEASE);
                  __atomic_store_n(&thread->migration_state, SCHED_MIGRATION_COMMITTED, __ATOMIC_RELEASE);
                  sched_remote_respond(&envelope, SCHED_COMPLETION_ACK, 0);
              } else if (mig_state == SCHED_MIGRATION_ROLLBACK_SENT) {
                  // Rollback enqueue accepted on old owner
                  __atomic_store_n(&thread->owner_cpu, core, __ATOMIC_RELEASE);
                  thread->bound_core_id = core;
                  __atomic_store_n(&thread->owner_state, THREAD_OWNER_NONE, __ATOMIC_RELEASE);
                  __atomic_store_n(&thread->migration_state, SCHED_MIGRATION_NONE, __ATOMIC_RELEASE);
                  sched_remote_respond(&envelope, SCHED_COMPLETION_ACK, 0);
              } else {
                  sched_remote_respond(&envelope, SCHED_COMPLETION_NACK, -6);
                  continue;
              }
          } else if (envelope.type == SCHED_REMOTE_DEQUEUE) {
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
              sched_remote_respond(&envelope, SCHED_COMPLETION_ACK, 0);
              continue; // DEQUEUE is complete
          } else if (envelope.type == SCHED_REMOTE_QUARANTINE) {
              sched_quarantine_thread(thread, envelope.flags);
              sched_remote_respond(&envelope, SCHED_COMPLETION_ACK, 0);
              continue;
          } else if (envelope.type == SCHED_REMOTE_SET_PRIORITY) {
              if (slot->is_on_runqueue != 0U) {
                  sched_invariant_on_dequeue(thread);
                  if (g_policy == SCHED_POLICY_CLOUD_FAIR) {
                      sched_cfs_dequeue(rq, thread);
                  } else if (g_policy == SCHED_POLICY_EDF) {
                      sched_edf_dequeue(rq, thread);
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
              thread->priority = envelope.priority;
              if (thread->state == THREAD_STATE_READY) {
                  sched_invariant_on_enqueue(thread, core);
                  if (g_policy == SCHED_POLICY_CLOUD_FAIR) {
                      sched_cfs_enqueue(rq, thread);
                  } else if (g_policy == SCHED_POLICY_EDF) {
                      sched_edf_enqueue(rq, thread);
                  } else {
                      list_add(&slot->run_node, &rq->ready_queue[thread->priority]);
                      sched_ready_bitmap_set(rq, thread->priority);
                  }
                  slot->is_on_runqueue = 1U;
                  rq->runnable_count++;
              }
              sched_remote_respond(&envelope, SCHED_COMPLETION_ACK, 0);
              continue;
          } else if (envelope.type == SCHED_REMOTE_SET_THROTTLE) {
              rq->throttled = envelope.flags;
              sched_remote_respond(&envelope, SCHED_COMPLETION_ACK, 0);
              continue;
          } else if (envelope.type == SCHED_REMOTE_TERMINATE) {
              sched_mark_thread_terminated(thread);
              sched_remote_respond(&envelope, SCHED_COMPLETION_ACK, 0);
              continue;
          } else if (envelope.type == SCHED_REMOTE_REAP) {
              sched_enqueue_reap(slot);
              sched_remote_respond(&envelope, SCHED_COMPLETION_ACK, 0);
              continue;
          } else if (envelope.type == SCHED_REMOTE_HANDOFF) {
              sched_request_handoff_tid(thread->thread_id, envelope.target_cpu, envelope.flags);
              sched_remote_respond(&envelope, SCHED_COMPLETION_ACK, 0);
              continue;
          } else if (envelope.type == SCHED_REMOTE_SET_AFFINITY) {
              thread->affinity_mask = envelope.flags;
              sched_remote_respond(&envelope, SCHED_COMPLETION_ACK, 0);
              continue;
          } else if (envelope.type == SCHED_REMOTE_SET_CONSTRAINTS) {
              thread->constraints = envelope.constraints;
              sched_remote_respond(&envelope, SCHED_COMPLETION_ACK, 0);
              continue;
          }

          if (thread->state == THREAD_STATE_QUARANTINED || thread->owner_state == THREAD_OWNER_QUARANTINED) {
              if (thread->migration_state == SCHED_MIGRATION_ROLLBACK_SENT) {
                  thread->migration_state = SCHED_MIGRATION_FAILED;
                  thread->pending_fault = THREAD_FAULT_MIGRATION_ROLLBACK_FAILED;
              }
              sched_remote_respond(&envelope, SCHED_COMPLETION_NACK, -7);
              continue;
          }

          if (envelope.type == SCHED_REMOTE_WAKE) {
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
          sched_remote_respond(&envelope, SCHED_COMPLETION_ACK, 0);
      }
      if (drained > 0) {
          rq->inbox_drains++;
          if (rq->current_thread && highest_prio_arrived &&
              highest_prio_arrived->priority > rq->current_thread->priority) {
              rq->remote_preemptions++;
          }
      }
      spin_unlock(&rq->lock);
      sched_publish_load(rq);
  }

  if (g_cpu_locals[core].runqueue.throttled != 0U && g_cpu_locals[core].runqueue.idle_thread) {
    sched_publish_load(rq);
    sched_switch_to(g_cpu_locals[core].runqueue.idle_thread, core);
    return;
  }

  bh_thread_t *next = sched_pick_next_ready(core);
  sched_publish_load(rq);
  sched_switch_to(next, core);
}

void sched_on_timer_tick(void) {
  sched_remote_cmd_poll_timeouts();
  g_cpu_locals[sched_clamp_core(hal_cpu_get_id())].runqueue.total_ticks++;


  uint32_t core = sched_clamp_core(hal_cpu_get_id());
  sched_publish_load(&g_cpu_locals[core].runqueue);

  ipc_async_check_timeouts(g_cpu_locals[core].runqueue.total_ticks);

  list_head_t *sleep_head = &g_cpu_locals[core].runqueue.sleeping_list;
  list_head_t *curr = sleep_head->next;
  while (curr != sleep_head) {
    thread_slot_t *slot = (thread_slot_t *)(void *)((char *)curr - offsetof(thread_slot_t, wait_node));
    curr = curr->next;
    if (slot->thread.state == THREAD_STATE_SLEEPING &&
        slot->thread.wake_deadline_ms <= g_cpu_locals[core].runqueue.total_ticks) {
      sched_wake_tid(slot->thread.thread_id);
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
      sched_wake_tid(slot->thread.thread_id);
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

          thread_slot_t *slot = sched_find_thread_slot_by_tid(current->thread_id);
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

sched_rq_t *sched_local_rq(void) {
  uint32_t core = sched_clamp_core(hal_cpu_get_id());
  return &g_cpu_locals[core].runqueue;
}

void sched_assert_local_rq(sched_rq_t *rq) {
  uint32_t core = sched_clamp_core(hal_cpu_get_id());
  if (rq != &g_cpu_locals[core].runqueue) {
    kernel_panic("sched_assert_local_rq failed: mutation of remote runqueue");
  }
}

sched_remote_cmd_t *sched_allocate_outbound_cmd(void) {
  uint32_t core = sched_clamp_core(hal_cpu_get_id());
  sched_rq_t *rq = &g_cpu_locals[core].runqueue;
  uint32_t slot_idx = 0xFFFF;

  for (uint32_t w = 0; w < SCHED_CMD_BITMAP_WORDS; ++w) {
    uint32_t val = __atomic_load_n(&rq->outbound_alloc_bitmap[w], __ATOMIC_ACQUIRE);
    while (val != 0xFFFFFFFFU) {
      uint32_t free_bit = __builtin_ctz(~val);
      uint32_t new_val = val | (1U << free_bit);
      if (__atomic_compare_exchange_n(&rq->outbound_alloc_bitmap[w], &val, new_val, false, __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE)) {
        slot_idx = w * 32 + free_bit;
        break;
      }
    }
    if (slot_idx != 0xFFFF) {
      break;
    }
  }

  if (slot_idx >= SCHED_REMOTE_CMD_CAPACITY) {
    return NULL;
  }

  sched_remote_cmd_t *cmd = &rq->outbound_cmds[slot_idx];

  // Increment generation (start of RESERVED state)
  cmd->handle.generation++;
  if (cmd->handle.generation == 0) {
    cmd->handle.generation = 1;
  }

  cmd->cmd_id = slot_idx;
  cmd->type = (sched_remote_cmd_type_t)0;
  cmd->source_cpu = core;
  cmd->target_cpu = 0;
  cmd->thread_id = 0;
  cmd->expected_thread_generation = 0;
  cmd->flags = 0;
  cmd->priority = 0;
  cmd->migration_epoch = 0;
  cmd->result = 0;
  cmd->submit_tick = 0;
  cmd->deadline_tick = 0;
  list_init(&cmd->list);

  // Set state to RESERVED under memory barrier
  __atomic_store_n(&cmd->state, SCHED_REMOTE_CMD_RESERVED, __ATOMIC_RELEASE);

  return cmd;
}

void sched_remote_cmd_release(sched_remote_cmd_t *cmd) {
  if (!cmd) return;
  uint16_t slot = cmd->handle.slot;
  uint32_t w = slot / 32;
  uint32_t bit = slot % 32;
  uint32_t mask = ~(1U << bit);

  // Set state to EMPTY
  __atomic_store_n(&cmd->state, SCHED_REMOTE_CMD_EMPTY, __ATOMIC_RELEASE);

  // Clear from bitmap
  uint32_t core = cmd->handle.origin_cpu;
  sched_rq_t *rq = &g_cpu_locals[core].runqueue;
  __atomic_fetch_and(&rq->outbound_alloc_bitmap[w], mask, __ATOMIC_ACQ_REL);
}

kstatus_t sched_remote_submit(uint32_t target_cpu, const sched_remote_cmd_t *cmd) {
  if (target_cpu >= g_active_core_count) {
    return K_ERR_INVALID_ARG;
  }
  uint32_t current_core = sched_clamp_core(hal_cpu_get_id());
  if (target_cpu == current_core) {
    return K_ERR_INVALID_ARG;
  }

  sched_rq_t *rq = &g_cpu_locals[current_core].runqueue;
  sched_rq_t *target_rq = &g_cpu_locals[target_cpu].runqueue;
  sched_remote_cmd_t *mutable_cmd = (sched_remote_cmd_t *)cmd;

  mutable_cmd->submit_tick = rq->total_ticks;
  mutable_cmd->deadline_tick = rq->total_ticks + 10U; // 10 ticks deadline
  __atomic_store_n(&mutable_cmd->state, SCHED_REMOTE_CMD_PENDING, __ATOMIC_RELEASE);

  sched_remote_cmd_envelope_t envelope;
  envelope.handle = cmd->handle;
  envelope.type = cmd->type;
  envelope.source_cpu = cmd->source_cpu;
  envelope.target_cpu = cmd->target_cpu;
  envelope.thread_id = cmd->thread_id;
  envelope.expected_thread_generation = cmd->expected_thread_generation;
  envelope.migration_epoch = cmd->migration_epoch;
  envelope.flags = cmd->flags;
  envelope.priority = cmd->priority;
  envelope.constraints = cmd->constraints;

  kstatus_t status = sched_cmd_ring_push(&target_rq->remote.cmd_ring, &envelope);
  if (status != K_OK) {
    __atomic_store_n(&mutable_cmd->state, SCHED_REMOTE_CMD_RESERVED, __ATOMIC_RELEASE);
    __atomic_fetch_add(&target_rq->remote.full, 1, __ATOMIC_RELAXED);
    return K_ERR_NO_RESOURCES;
  }

  __atomic_fetch_add(&target_rq->remote.submitted, 1, __ATOMIC_RELAXED);

  uint32_t expected = 0;
  if (__atomic_compare_exchange_n(&target_rq->remote.resched_pending, &expected, 1, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST)) {
      __atomic_fetch_add(&target_rq->remote.ipi_sent, 1, __ATOMIC_RELAXED);
      uint64_t msg = MK_MSG_THREAD_ENQUEUE_REQ;
      if (cmd->type == SCHED_REMOTE_WAKE) {
          msg = MK_MSG_THREAD_WAKE_REQ;
      } else if (cmd->type == SCHED_REMOTE_MIGRATE || cmd->type == SCHED_REMOTE_MIGRATE_PREPARE) {
          msg = MK_MSG_THREAD_DEQUEUE_REQ;
      }
      hal_send_ipi_payload(1U << target_cpu, msg);
  } else {
      __atomic_fetch_add(&target_rq->remote.ipi_coalesced, 1, __ATOMIC_RELAXED);
  }

  return K_OK;
}

kstatus_t sched_read_load_snapshot(uint32_t cpu, sched_load_snapshot_t *out) {
  if (cpu >= g_active_core_count || !out) return K_ERR_INVALID_ARG;
  sched_rq_t *rq = &g_cpu_locals[cpu].runqueue;
  uint32_t seq;
  do {
    seq = __atomic_load_n(&rq->load_snapshot.load_seq, __ATOMIC_ACQUIRE);
    out->runnable_count = __atomic_load_n(&rq->load_snapshot.runnable_count, __ATOMIC_ACQUIRE);
  } while ((seq & 1) != 0 || seq != __atomic_load_n(&rq->load_snapshot.load_seq, __ATOMIC_ACQUIRE));
  out->load_seq = seq;
  return K_OK;
}

bool sched_read_isolated_snapshot(uint32_t cpu) {
  if (cpu >= g_active_core_count) return false;
  sched_rq_t *rq = &g_cpu_locals[cpu].runqueue;
  return __atomic_load_n(&rq->sched_isolated, __ATOMIC_ACQUIRE);
}

kstatus_t sched_migration_transition(bh_thread_t *thread, sched_migration_state_t expected, sched_migration_state_t next) {
  if (!thread) return K_ERR_INVALID_ARG;
  uint32_t state = __atomic_load_n(&thread->migration_state, __ATOMIC_ACQUIRE);
  if (state != (uint32_t)expected) {
#if !defined(NDEBUG)
    kernel_panic("sched_migration_transition failed: expected %d, got %d", expected, state);
#else
    // Production safety fallback: reject transition, quarantine if ambiguous
    if (expected == SCHED_MIGRATION_ROLLBACK_SENT || next == SCHED_MIGRATION_FAILED) {
        sched_quarantine_thread(thread, THREAD_FAULT_MIGRATION_ROLLBACK_FAILED);
    }
    return K_ERR_BAD_STATE;
#endif
  }
  __atomic_store_n(&thread->migration_state, (uint32_t)next, __ATOMIC_RELEASE);
  return K_OK;
}

void sched_remote_cmd_poll_timeouts(void) {
  uint32_t core = sched_clamp_core(hal_cpu_get_id());
  sched_rq_t *rq = &g_cpu_locals[core].runqueue;
  uint64_t current_ticks = rq->total_ticks;

  // 1. Scan for timeouts
  for (uint32_t i = 0; i < SCHED_REMOTE_CMD_CAPACITY; ++i) {
    sched_remote_cmd_t *cmd = &rq->outbound_cmds[i];
    uint32_t state = __atomic_load_n(&cmd->state, __ATOMIC_ACQUIRE);

    if (state == SCHED_REMOTE_CMD_PENDING) {
      if (cmd->deadline_tick > 0 && current_ticks >= cmd->deadline_tick) {
        __atomic_store_n(&cmd->state, SCHED_REMOTE_CMD_TIMEOUT, __ATOMIC_RELEASE);
      }
    }
  }

  // 2. Process all completions in our ring
  sched_remote_completion_t completion;
  while (sched_completion_ring_pop(&rq->remote.completion_ring, &completion) == K_OK) {
    uint16_t slot_idx = completion.handle.slot;
    if (slot_idx >= SCHED_REMOTE_CMD_CAPACITY) {
      continue;
    }
    sched_remote_cmd_t *cmd = &rq->outbound_cmds[slot_idx];

    // Authoritative generation validation:
    if (cmd->handle.generation != completion.handle.generation) {
      // Stale completion from an old, timed-out command. Ignore completely!
      continue;
    }

    uint32_t current_state = __atomic_load_n(&cmd->state, __ATOMIC_ACQUIRE);
    if (current_state != SCHED_REMOTE_CMD_PENDING) {
      // Command already retired/timed out or duplicate. Ignore!
      continue;
    }

    // Update state based on completion result
    uint32_t next_state = (completion.kind == SCHED_COMPLETION_ACK) ? SCHED_REMOTE_CMD_ACKED : SCHED_REMOTE_CMD_FAILED;
    cmd->result = completion.result;
    __atomic_store_n(&cmd->state, next_state, __ATOMIC_RELEASE);
  }

  // 3. Process actions on finalized/terminal commands
  for (uint32_t i = 0; i < SCHED_REMOTE_CMD_CAPACITY; ++i) {
    sched_remote_cmd_t *cmd = &rq->outbound_cmds[i];
    uint32_t state = __atomic_load_n(&cmd->state, __ATOMIC_ACQUIRE);

    if (state == SCHED_REMOTE_CMD_ACKED || state == SCHED_REMOTE_CMD_FAILED || state == SCHED_REMOTE_CMD_TIMEOUT) {
      // Terminal state observed by originating CPU
      bh_thread_t *thread = sched_find_thread_by_id(cmd->thread_id);
      if (thread) {
        // Validate generation and epoch
        if (thread->sched_generation == cmd->expected_thread_generation &&
            thread->migration_epoch == cmd->migration_epoch) {

          if (cmd->type == SCHED_REMOTE_MIGRATE_PREPARE) {
            if (state == SCHED_REMOTE_CMD_ACKED) {
              // Prepare (dequeue) succeeded on old owner. Transition DEQUEUED -> COMMIT_SENT
              if (sched_migration_transition(thread, SCHED_MIGRATION_DEQUEUED, SCHED_MIGRATION_COMMIT_SENT) == K_OK) {
                // Submit commit/enqueue command to target_cpu
                sched_remote_cmd_t *commit_cmd = sched_allocate_outbound_cmd();
                if (commit_cmd) {
                  commit_cmd->type = SCHED_REMOTE_ENQUEUE;
                  commit_cmd->source_cpu = core;
                  commit_cmd->target_cpu = thread->migration_target_cpu;
                  commit_cmd->thread_id = thread->thread_id;
                  commit_cmd->expected_thread_generation = thread->sched_generation;
                  commit_cmd->migration_epoch = thread->migration_epoch;
                  commit_cmd->priority = thread->priority;
                  commit_cmd->state = SCHED_REMOTE_CMD_PENDING;

                  kstatus_t status = sched_remote_submit(thread->migration_target_cpu, commit_cmd);
                  if (status != K_OK) {
                    sched_remote_cmd_release(commit_cmd);
                    // Commit submission failed! Roll back.
                    sched_migration_transition(thread, SCHED_MIGRATION_COMMIT_SENT, SCHED_MIGRATION_ROLLBACK_SENT);
                    // Send rollback to old owner to re-enqueue
                    sched_remote_cmd_t *rb_cmd = sched_allocate_outbound_cmd();
                    if (rb_cmd) {
                      rb_cmd->type = SCHED_REMOTE_ENQUEUE;
                      rb_cmd->source_cpu = core;
                      rb_cmd->target_cpu = cmd->target_cpu; // old owner
                      rb_cmd->thread_id = thread->thread_id;
                      rb_cmd->expected_thread_generation = thread->sched_generation;
                      rb_cmd->migration_epoch = thread->migration_epoch;
                      rb_cmd->state = SCHED_REMOTE_CMD_PENDING;
                      kstatus_t st = sched_remote_submit(cmd->target_cpu, rb_cmd);
                      if (st != K_OK) {
                        sched_remote_cmd_release(rb_cmd);
                        sched_quarantine_thread(thread, THREAD_FAULT_MIGRATION_ROLLBACK_FAILED);
                        sched_migration_transition(thread, SCHED_MIGRATION_ROLLBACK_SENT, SCHED_MIGRATION_FAILED);
                      }
                    } else {
                      sched_quarantine_thread(thread, THREAD_FAULT_MIGRATION_ROLLBACK_FAILED);
                      sched_migration_transition(thread, SCHED_MIGRATION_ROLLBACK_SENT, SCHED_MIGRATION_FAILED);
                    }
                  }
                } else {
                  // Commit alloc failed -> rollback
                  sched_migration_transition(thread, SCHED_MIGRATION_DEQUEUED, SCHED_MIGRATION_ROLLBACK_SENT);
                  sched_remote_cmd_t *rb_cmd = sched_allocate_outbound_cmd();
                  if (rb_cmd) {
                    rb_cmd->type = SCHED_REMOTE_ENQUEUE;
                    rb_cmd->source_cpu = core;
                    rb_cmd->target_cpu = cmd->target_cpu;
                    rb_cmd->thread_id = thread->thread_id;
                    rb_cmd->expected_thread_generation = thread->sched_generation;
                    rb_cmd->migration_epoch = thread->migration_epoch;
                    rb_cmd->state = SCHED_REMOTE_CMD_PENDING;
                    kstatus_t st = sched_remote_submit(cmd->target_cpu, rb_cmd);
                    if (st != K_OK) {
                      sched_remote_cmd_release(rb_cmd);
                      sched_quarantine_thread(thread, THREAD_FAULT_MIGRATION_ROLLBACK_FAILED);
                      sched_migration_transition(thread, SCHED_MIGRATION_ROLLBACK_SENT, SCHED_MIGRATION_FAILED);
                    }
                  } else {
                    sched_quarantine_thread(thread, THREAD_FAULT_MIGRATION_ROLLBACK_FAILED);
                    sched_migration_transition(thread, SCHED_MIGRATION_ROLLBACK_SENT, SCHED_MIGRATION_FAILED);
                  }
                }
              }
            } else {
              // Prepare failed or timed out! Restore to NONE.
              sched_migration_transition(thread, SCHED_MIGRATION_PREPARE_SENT, SCHED_MIGRATION_NONE);
            }
          } else if (cmd->type == SCHED_REMOTE_ENQUEUE) {
            // COMMIT phase
            if (state == SCHED_REMOTE_CMD_ACKED) {
              // Commit succeeded.
              uint32_t current_m_state = __atomic_load_n(&thread->migration_state, __ATOMIC_ACQUIRE);
              if (current_m_state == SCHED_MIGRATION_COMMITTED) {
                sched_migration_transition(thread, SCHED_MIGRATION_COMMITTED, SCHED_MIGRATION_NONE);
              } else if (current_m_state == SCHED_MIGRATION_ROLLBACK_SENT) {
                sched_migration_transition(thread, SCHED_MIGRATION_ROLLBACK_SENT, SCHED_MIGRATION_NONE);
              }
            } else {
              // Commit failed or timed out!
              uint32_t current_m_state = __atomic_load_n(&thread->migration_state, __ATOMIC_ACQUIRE);
              if (current_m_state == SCHED_MIGRATION_COMMIT_SENT) {
                sched_migration_transition(thread, SCHED_MIGRATION_COMMIT_SENT, SCHED_MIGRATION_ROLLBACK_SENT);
                // Send rollback to old owner
                sched_remote_cmd_t *rb_cmd = sched_allocate_outbound_cmd();
                if (rb_cmd) {
                  rb_cmd->type = SCHED_REMOTE_ENQUEUE;
                  rb_cmd->source_cpu = core;
                  rb_cmd->target_cpu = cmd->source_cpu; // old owner (source_cpu)
                  rb_cmd->thread_id = thread->thread_id;
                  rb_cmd->expected_thread_generation = thread->sched_generation;
                  rb_cmd->migration_epoch = thread->migration_epoch;
                  rb_cmd->state = SCHED_REMOTE_CMD_PENDING;
                  kstatus_t st = sched_remote_submit(cmd->source_cpu, rb_cmd);
                  if (st != K_OK) {
                    sched_remote_cmd_release(rb_cmd);
                    sched_quarantine_thread(thread, THREAD_FAULT_MIGRATION_ROLLBACK_FAILED);
                    sched_migration_transition(thread, SCHED_MIGRATION_ROLLBACK_SENT, SCHED_MIGRATION_FAILED);
                  }
                } else {
                  sched_quarantine_thread(thread, THREAD_FAULT_MIGRATION_ROLLBACK_FAILED);
                  sched_migration_transition(thread, SCHED_MIGRATION_ROLLBACK_SENT, SCHED_MIGRATION_FAILED);
                }
              } else if (current_m_state == SCHED_MIGRATION_ROLLBACK_SENT) {
                // Rollback failed or timed out -> QUARANTINE
                sched_quarantine_thread(thread, THREAD_FAULT_MIGRATION_ROLLBACK_FAILED);
                sched_migration_transition(thread, SCHED_MIGRATION_ROLLBACK_SENT, SCHED_MIGRATION_FAILED);
              }
            }
          }
        }
      }
      // Re-release command slot
      sched_remote_cmd_release(cmd);
    }
  }
}

kstatus_t sched_cmd_ring_init(sched_cmd_ring_t *q, sched_cmd_slot_t *slots, uint32_t capacity) {
    if (!q || !slots || capacity < 2 || (capacity & (capacity - 1)) != 0) {
        return K_ERR_INVALID_ARG;
    }
    q->slots = slots;
    q->capacity = capacity;
    q->mask = capacity - 1;
    q->head = 0;
    q->tail = 0;

    for (uint32_t i = 0; i < capacity; i++) {
        q->slots[i].seq = i;
        __builtin_memset(&q->slots[i].value, 0, sizeof(sched_remote_cmd_envelope_t));
    }
    return K_OK;
}

kstatus_t sched_cmd_ring_push(sched_cmd_ring_t *q, const sched_remote_cmd_envelope_t *value) {
    if (!q || !value) return K_ERR_INVALID_ARG;
    sched_cmd_slot_t *slot;
    uint64_t pos = q->head;

    while (true) {
        slot = &q->slots[pos & q->mask];
        uint64_t seq = __atomic_load_n(&slot->seq, __ATOMIC_ACQUIRE);
        int64_t diff = (int64_t)seq - (int64_t)pos;

        if (diff == 0) {
            if (__atomic_compare_exchange_n(&q->head, &pos, pos + 1, false, __ATOMIC_RELAXED, __ATOMIC_RELAXED)) {
                break;
            }
        } else if (diff < 0) {
            return K_ERR_AGAIN;
        } else {
            pos = __atomic_load_n(&q->head, __ATOMIC_RELAXED);
        }
    }

    slot->value = *value;
    __atomic_store_n(&slot->seq, pos + 1, __ATOMIC_RELEASE);
    return K_OK;
}

kstatus_t sched_cmd_ring_pop(sched_cmd_ring_t *q, sched_remote_cmd_envelope_t *out_value) {
    if (!q) return K_ERR_INVALID_ARG;
    sched_cmd_slot_t *slot;
    uint64_t pos = q->tail;

    slot = &q->slots[pos & q->mask];
    uint64_t seq = __atomic_load_n(&slot->seq, __ATOMIC_ACQUIRE);
    int64_t diff = (int64_t)seq - (int64_t)(pos + 1);

    if (diff == 0) {
        q->tail = pos + 1;
        if (out_value) {
            *out_value = slot->value;
        }
        __atomic_store_n(&slot->seq, pos + q->mask + 1, __ATOMIC_RELEASE);
        return K_OK;
    }
    return K_ERR_AGAIN;
}

bool sched_cmd_ring_empty(const sched_cmd_ring_t *q) {
    if (!q) return true;
    uint64_t head = __atomic_load_n(&q->head, __ATOMIC_RELAXED);
    return q->tail == head;
}

kstatus_t sched_completion_ring_init(sched_completion_ring_t *q, sched_completion_slot_t *slots, uint32_t capacity) {
    if (!q || !slots || capacity < 2 || (capacity & (capacity - 1)) != 0) {
        return K_ERR_INVALID_ARG;
    }
    q->slots = slots;
    q->capacity = capacity;
    q->mask = capacity - 1;
    q->head = 0;
    q->tail = 0;

    for (uint32_t i = 0; i < capacity; i++) {
        q->slots[i].seq = i;
        __builtin_memset(&q->slots[i].value, 0, sizeof(sched_remote_completion_t));
    }
    return K_OK;
}

kstatus_t sched_completion_ring_push(sched_completion_ring_t *q, const sched_remote_completion_t *value) {
    if (!q || !value) return K_ERR_INVALID_ARG;
    sched_completion_slot_t *slot;
    uint64_t pos = q->head;

    while (true) {
        slot = &q->slots[pos & q->mask];
        uint64_t seq = __atomic_load_n(&slot->seq, __ATOMIC_ACQUIRE);
        int64_t diff = (int64_t)seq - (int64_t)pos;

        if (diff == 0) {
            if (__atomic_compare_exchange_n(&q->head, &pos, pos + 1, false, __ATOMIC_RELAXED, __ATOMIC_RELAXED)) {
                break;
            }
        } else if (diff < 0) {
            return K_ERR_AGAIN;
        } else {
            pos = __atomic_load_n(&q->head, __ATOMIC_RELAXED);
        }
    }

    slot->value = *value;
    __atomic_store_n(&slot->seq, pos + 1, __ATOMIC_RELEASE);
    return K_OK;
}

kstatus_t sched_completion_ring_pop(sched_completion_ring_t *q, sched_remote_completion_t *out_value) {
    if (!q) return K_ERR_INVALID_ARG;
    sched_completion_slot_t *slot;
    uint64_t pos = q->tail;

    slot = &q->slots[pos & q->mask];
    uint64_t seq = __atomic_load_n(&slot->seq, __ATOMIC_ACQUIRE);
    int64_t diff = (int64_t)seq - (int64_t)(pos + 1);

    if (diff == 0) {
        q->tail = pos + 1;
        if (out_value) {
            *out_value = slot->value;
        }
        __atomic_store_n(&slot->seq, pos + q->mask + 1, __ATOMIC_RELEASE);
        return K_OK;
    }
    return K_ERR_AGAIN;
}

bool sched_completion_ring_empty(const sched_completion_ring_t *q) {
    if (!q) return true;
    uint64_t head = __atomic_load_n(&q->head, __ATOMIC_RELAXED);
    return q->tail == head;
}
