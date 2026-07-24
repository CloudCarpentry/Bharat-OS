#include "sched/sched.h"
#include "sched_internal.h"
#include "arch/cpu_relax.h"

uint32_t sched_read_published_load(uint32_t core_id) {
  if (core_id >= g_active_core_count) return 0;
  sched_rq_t *rq = &g_cpu_locals[core_id].runqueue;
  uint32_t seq, count;
  do {
    seq = __atomic_load_n(&rq->load_snapshot.load_seq, __ATOMIC_ACQUIRE);
    count = __atomic_load_n(&rq->load_snapshot.runnable_count, __ATOMIC_ACQUIRE);
  } while ((seq & 1) != 0 || seq != __atomic_load_n(&rq->load_snapshot.load_seq, __ATOMIC_ACQUIRE));
  return count;
}

bh_thread_t *sched_find_steal_candidate(uint32_t core_id, uint32_t target_cpu) {
  sched_rq_t *rq = &g_cpu_locals[core_id].runqueue;

  for (uint32_t p = 0; p < MAX_PRIORITY_LEVELS; ++p) {
    list_head_t *head = &rq->ready_queue[p];
    list_head_t *curr = head->next;
    while (curr != head) {
      thread_slot_t *slot = (thread_slot_t *)(void *)((char *)curr - offsetof(thread_slot_t, run_node));
      curr = curr->next;
      bh_thread_t *t = &slot->thread;

      if (t == rq->current_thread || t == rq->idle_thread) continue;
      if (t->state != THREAD_STATE_READY) continue;
      if (t->migration_state != SCHED_MIGRATION_NONE) continue;
      if (t->owner_state == THREAD_OWNER_QUARANTINED) continue;
      if (slot->is_sleeping || slot->is_blocked) continue;

      if ((t->affinity_mask & (1U << target_cpu)) == 0U) continue;
      if (!sched_is_core_admissible(t, target_cpu)) continue;

      if (t->rt_attr.period_ms > 0 || t->rt_attr.deadline_ms > 0) continue;

      return t;
    }
  }
  return NULL;
}

void sched_balance_once(void) {
  uint32_t busiest = 0U;
  uint32_t idlest = 0U;
  uint32_t max_depth = 0U;
  uint32_t min_depth = UINT32_MAX;

  for (uint32_t core = 0; core < g_active_core_count; ++core) {
    uint32_t depth = sched_read_published_load(core);
    if (depth > max_depth) {
      max_depth = depth;
      busiest = core;
    }
    if (depth < min_depth) {
      min_depth = depth;
      idlest = core;
    }
  }

  if (max_depth <= (min_depth + 1U) || busiest == idlest) {
    return;
  }

  sched_remote_cmd_t *cmd = sched_allocate_outbound_cmd();
  if (cmd) {
      cmd->type = SCHED_REMOTE_STEAL_REQ;
      cmd->source_cpu = busiest;
      cmd->target_cpu = idlest;
      cmd->state = SCHED_REMOTE_CMD_PENDING;

      sched_remote_submit(busiest, cmd);
  }
}

int sched_migrate_task(bh_thread_t *thread, uint32_t new_node) {
  if (!thread || new_node >= g_active_core_count) {
    return -1;
  }
  if ((thread->affinity_mask & (1U << new_node)) == 0U) {
    return -2;
  }
  if (thread->state == THREAD_STATE_QUARANTINED || thread->owner_state == THREAD_OWNER_QUARANTINED) {
    return K_ERR_BAD_STATE;
  }

  if (thread->migration_state != SCHED_MIGRATION_NONE) {
      return K_ERR_IN_PROGRESS;
  }

  thread_slot_t *slot = sched_find_thread_slot_by_tid_local(&g_cpu_locals[thread->home_core_id].runqueue, thread->thread_id);
  if (!slot) {
    return -1;
  }

  uint32_t current_core = sched_clamp_core(hal_cpu_get_id());
  uint32_t bound_core = thread->bound_core_id;

  if (bound_core == new_node) {
      return 0;
  }

  thread->migration_target_cpu = new_node;

  if (bound_core == current_core) {
      // Phase 1: Local dequeue
      hal_cpu_disable_interrupts();
      sched_rq_t *rq = &g_cpu_locals[current_core].runqueue;
      if (slot->is_on_runqueue != 0U) {
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
      thread->migration_state = SCHED_MIGRATION_DEQUEUED;
      thread->owner_state = THREAD_OWNER_REMOTE_PENDING;
      hal_cpu_enable_interrupts();

      // Phase 2: Remote enqueue request
      sched_remote_cmd_t *cmd = sched_allocate_outbound_cmd();
      if (!cmd) {
          hal_cpu_disable_interrupts();
          thread->owner_state = THREAD_OWNER_NONE;
          thread->migration_state = SCHED_MIGRATION_NONE;
          sched_invariant_on_enqueue(thread, current_core);
          if (g_policy == SCHED_POLICY_CLOUD_FAIR) {
            sched_cfs_enqueue(rq, thread);
          } else {
            list_add(&slot->run_node, &rq->ready_queue[thread->priority]);
            sched_ready_bitmap_set(rq, thread->priority);
          }
          slot->is_on_runqueue = 1U;
          rq->runnable_count++;
          hal_cpu_enable_interrupts();
          return K_ERR_NO_RESOURCES;
      }

      cmd->type = SCHED_REMOTE_ENQUEUE;
      cmd->source_cpu = current_core;
      cmd->target_cpu = new_node;
      cmd->thread_id = thread->thread_id;
      cmd->expected_thread_generation = thread->sched_generation;
      cmd->priority = thread->priority;
      cmd->state = SCHED_REMOTE_CMD_PENDING;

      thread->migration_state = SCHED_MIGRATION_ENQUEUE_REQUESTED;
      thread->preferred_numa_node = (uint8_t)new_node;

      kstatus_t status = sched_remote_submit(new_node, cmd);
      if (status != K_OK) {
          cmd->state = SCHED_REMOTE_CMD_EMPTY;
          hal_cpu_disable_interrupts();
          thread->owner_state = THREAD_OWNER_NONE;
          thread->migration_state = SCHED_MIGRATION_NONE;
          sched_invariant_on_enqueue(thread, current_core);
          if (g_policy == SCHED_POLICY_CLOUD_FAIR) {
            sched_cfs_enqueue(rq, thread);
          } else {
            list_add(&slot->run_node, &rq->ready_queue[thread->priority]);
            sched_ready_bitmap_set(rq, thread->priority);
          }
          slot->is_on_runqueue = 1U;
          rq->runnable_count++;
          hal_cpu_enable_interrupts();
          return status;
      }
      return K_ERR_IN_PROGRESS;
  } else {
      // Phase 1: Remote dequeue request (to old owner A)
      sched_remote_cmd_t *cmd = sched_allocate_outbound_cmd();
      if (!cmd) {
          return K_ERR_NO_RESOURCES;
      }

      cmd->type = SCHED_REMOTE_MIGRATE_PREPARE;
      cmd->source_cpu = current_core;
      cmd->target_cpu = bound_core;
      cmd->thread_id = thread->thread_id;
      cmd->expected_thread_generation = thread->sched_generation;
      cmd->state = SCHED_REMOTE_CMD_PENDING;

      thread->migration_state = SCHED_MIGRATION_DEQUEUE_REQUESTED;
      thread->preferred_numa_node = (uint8_t)new_node;

      kstatus_t status = sched_remote_submit(bound_core, cmd);
      if (status != K_OK) {
          cmd->state = SCHED_REMOTE_CMD_EMPTY;
          thread->migration_state = SCHED_MIGRATION_NONE;
          return status;
      }
      return K_ERR_IN_PROGRESS;
  }
}

int sched_set_thread_preferred_node(uint64_t tid, uint8_t node_id) {
  return sched_migrate_task(sched_find_thread_by_id(tid), node_id);
}

int sched_sys_set_affinity(uint64_t tid, uint32_t affinity_mask) {
  bh_thread_t *thread = sched_find_thread_by_id(tid);
  if (!thread || affinity_mask == 0U) {
    return -1;
  }
  thread->affinity_mask = affinity_mask;

  uint32_t current_core = thread->bound_core_id;
  if ((affinity_mask & (1U << current_core)) == 0U) {
    for (uint32_t core = 0; core < g_active_core_count; ++core) {
      if ((affinity_mask & (1U << core)) != 0U) {
        return sched_migrate_task(thread, core);
      }
    }
    return -1;
  }
  return 0;
}

int sched_throttle_core(uint32_t core_id) {
  if (core_id >= g_active_core_count) {
    return -1;
  }
  g_cpu_locals[core_id].runqueue.throttled = 1U;
  return 0;
}

kstatus_t sched_wait_remote_cmd_ack(sched_remote_cmd_t *cmd, uint32_t timeout_loops) {
    if (!cmd) return K_ERR_INVALID_ARG;
    for (uint32_t i = 0; i < timeout_loops; ++i) {
        if (cmd->state == SCHED_REMOTE_CMD_ACKED) return K_OK;
        if (cmd->state == SCHED_REMOTE_CMD_FAILED) return K_ERR_BAD_STATE;
        if (cmd->state == SCHED_REMOTE_CMD_TIMEOUT) return K_ERR_TIMEOUT;
        arch_cpu_relax();
    }
    return K_ERR_TIMEOUT;
}

int sched_quarantine_thread(bh_thread_t *thread, uint32_t reason) {
  if (!thread) return -1;

  thread->state = THREAD_STATE_QUARANTINED;
  thread->owner_state = THREAD_OWNER_QUARANTINED;
  thread->pending_fault = (thread_fault_t)reason;

  // If enqueued locally, remove it
  uint32_t current_core = sched_clamp_core(hal_cpu_get_id());
  if (thread->bound_core_id == current_core) {
      thread_slot_t *slot = sched_find_thread_slot_by_tid_local(&g_cpu_locals[current_core].runqueue, thread->thread_id);
      if (slot && slot->is_on_runqueue) {
          hal_cpu_disable_interrupts();
          sched_rq_t *rq = &g_cpu_locals[current_core].runqueue;
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
          hal_cpu_enable_interrupts();
      }
  }

  return 0;
}

int sched_request_remote_handoff(bh_thread_t *thread, uint32_t target_core, uint32_t auth_token) {
  if (!thread || target_core >= g_active_core_count) {
    return -1; // Invalid argument
  }

  uint32_t current_core = sched_clamp_core(hal_cpu_get_id());
  if (target_core == current_core) {
    return -1; // Cannot handoff to self
  }

  thread_slot_t *slot = sched_find_thread_slot_by_tid_local(&g_cpu_locals[thread->home_core_id].runqueue, thread->thread_id);
  if (!slot) {
    return -1;
  }

  hal_cpu_disable_interrupts();

  // Validate state - only READY threads can be handed off
  if (thread->state != THREAD_STATE_READY) {
    hal_cpu_enable_interrupts();
    return -2; // Invalid state
  }

  sched_rq_t *rq = &g_cpu_locals[current_core].runqueue;

  // Dequeue from local runqueue
  if (slot->is_on_runqueue != 0U) {
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

  // Transition to pending handoff state
  thread->state = THREAD_STATE_REMOTE_HANDOFF_PENDING;

  hal_cpu_enable_interrupts();

  // Prepare and send uRPC message
  mk_channel_t channel;
  if (mk_get_channel(current_core, target_core, &channel) != 0) {
    // If channel fails, revert state and re-enqueue locally
    hal_cpu_disable_interrupts();
    thread->state = THREAD_STATE_READY;
    if (g_policy == SCHED_POLICY_CLOUD_FAIR) {
      sched_cfs_enqueue(rq, thread);
    } else {
      list_add(&slot->run_node, &rq->ready_queue[thread->priority]);
      sched_ready_bitmap_set(rq, thread->priority);
    }
    slot->is_on_runqueue = 1U;
    rq->runnable_count++;
    hal_cpu_enable_interrupts();
    return -3; // Channel error
  }

  mk_msg_thread_handoff_t payload = {
    .thread_id = thread->thread_id,
    .source_core = current_core,
    .target_core = target_core,
    .priority = thread->priority,
    .flags = 0
  };

  urpc_msg_t msg = {
    .type = MK_MSG_THREAD_HANDOFF_REQ,
    .payload_size = sizeof(payload),
    .src_core = current_core,
    .dst_core = target_core,
    .auth_token = auth_token
  };
  __builtin_memcpy(msg.payload_data, &payload, sizeof(payload));

  int ret = mk_send_message(&channel, msg.type, msg.payload_data, msg.payload_size);
  if (ret != 0) {
      // If send fails, revert state and re-enqueue locally
      hal_cpu_disable_interrupts();
      thread->state = THREAD_STATE_READY;
      if (g_policy == SCHED_POLICY_CLOUD_FAIR) {
        sched_cfs_enqueue(rq, thread);
      } else {
        list_add(&slot->run_node, &rq->ready_queue[thread->priority]);
        sched_ready_bitmap_set(rq, thread->priority);
      }
      slot->is_on_runqueue = 1U;
      rq->runnable_count++;
      hal_cpu_enable_interrupts();
      return -4; // Send error
  }

  return 0;
}

