#include "sched/sched.h"
#include "sched_internal.h"
#include "arch/cpu_relax.h"
#include "panic.h"

bh_thread_t *sched_find_steal_candidate(uint32_t core_id, uint32_t target_cpu) {
  (void)core_id;
  sched_rq_t *rq = sched_local_rq();

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
    sched_load_snapshot_t snap;
    uint32_t depth = 0;
    if (sched_read_load_snapshot(core, &snap) == K_OK) {
        depth = snap.runnable_count;
    }
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

  uint32_t m_state = __atomic_load_n(&thread->migration_state, __ATOMIC_ACQUIRE);
  if (m_state != SCHED_MIGRATION_NONE) {
      return K_ERR_IN_PROGRESS;
  }

  thread_slot_t *slot = sched_find_thread_slot_by_tid(thread->thread_id);
  if (!slot) {
    return -1;
  }

  uint32_t current_core = sched_clamp_core(hal_cpu_get_id());
  uint32_t owner = __atomic_load_n(&thread->owner_cpu, __ATOMIC_ACQUIRE);

  if (owner == new_node) {
      return 0;
  }

  // Increment migration epoch at start of migration
  uint32_t epoch = __atomic_fetch_add(&thread->migration_epoch, 1, __ATOMIC_SEQ_CST) + 1;

  thread->migration_target_cpu = new_node;

  if (owner == current_core) {
      // Local owner: Phase 1: Local dequeue
      hal_cpu_disable_interrupts();
      sched_rq_t *rq = sched_local_rq();
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
      sched_migration_transition(thread, SCHED_MIGRATION_NONE, SCHED_MIGRATION_DEQUEUED);
      thread->owner_state = THREAD_OWNER_REMOTE_PENDING;
      hal_cpu_enable_interrupts();

      // Local owner: Phase 2: Remote enqueue request to new_node
      sched_remote_cmd_t *cmd = sched_allocate_outbound_cmd();
      if (!cmd) {
          hal_cpu_disable_interrupts();
          thread->owner_state = THREAD_OWNER_NONE;
          sched_migration_transition(thread, SCHED_MIGRATION_DEQUEUED, SCHED_MIGRATION_NONE);
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
      cmd->migration_epoch = epoch;
      cmd->priority = thread->priority;
      cmd->state = SCHED_REMOTE_CMD_PENDING;

      sched_migration_transition(thread, SCHED_MIGRATION_DEQUEUED, SCHED_MIGRATION_COMMIT_SENT);
      thread->preferred_numa_node = (uint8_t)new_node;

      kstatus_t status = sched_remote_submit(new_node, cmd);
      if (status != K_OK) {
          sched_remote_cmd_release(cmd);
          hal_cpu_disable_interrupts();
          thread->owner_state = THREAD_OWNER_NONE;
          sched_migration_transition(thread, SCHED_MIGRATION_COMMIT_SENT, SCHED_MIGRATION_NONE);
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
      // Remote owner: Send prepare command to owner core
      sched_remote_cmd_t *cmd = sched_allocate_outbound_cmd();
      if (!cmd) {
          return K_ERR_NO_RESOURCES;
      }

      cmd->type = SCHED_REMOTE_MIGRATE_PREPARE;
      cmd->source_cpu = current_core;
      cmd->target_cpu = owner;
      cmd->thread_id = thread->thread_id;
      cmd->expected_thread_generation = thread->sched_generation;
      cmd->migration_epoch = epoch;
      cmd->state = SCHED_REMOTE_CMD_PENDING;

      sched_migration_transition(thread, SCHED_MIGRATION_NONE, SCHED_MIGRATION_PREPARE_SENT);
      thread->preferred_numa_node = (uint8_t)new_node;

      kstatus_t status = sched_remote_submit(owner, cmd);
      if (status != K_OK) {
          sched_remote_cmd_release(cmd);
          sched_migration_transition(thread, SCHED_MIGRATION_PREPARE_SENT, SCHED_MIGRATION_NONE);
          return status;
      }
      return K_ERR_IN_PROGRESS;
  }
}

int sched_migrate_tid(uint64_t tid, uint32_t target_cpu) {
  bh_thread_t *thread = sched_find_thread_by_id(tid);
  if (!thread) return -1;
  return sched_migrate_task(thread, target_cpu);
}

int sched_set_thread_preferred_node(uint64_t tid, uint8_t node_id) {
  return sched_migrate_tid(tid, node_id);
}

int sched_set_affinity(uint64_t tid, uint32_t mask) {
  bh_thread_t *thread = sched_find_thread_by_id(tid);
  if (!thread || mask == 0U) {
    return -1;
  }

  uint32_t current_core = sched_clamp_core(hal_cpu_get_id());
  uint32_t owner = __atomic_load_n(&thread->owner_cpu, __ATOMIC_ACQUIRE);

  if (owner != current_core) {
      sched_remote_cmd_t *cmd = sched_allocate_outbound_cmd();
      if (!cmd) return K_ERR_NO_RESOURCES;
      cmd->type = SCHED_REMOTE_SET_AFFINITY;
      cmd->source_cpu = current_core;
      cmd->target_cpu = owner;
      cmd->thread_id = thread->thread_id;
      cmd->expected_thread_generation = thread->sched_generation;
      cmd->flags = mask; // pass affinity mask here
      cmd->state = SCHED_REMOTE_CMD_PENDING;

      kstatus_t status = sched_remote_submit(owner, cmd);
      if (status != K_OK) {
          sched_remote_cmd_release(cmd);
          return status;
      }
      return 0;
  }

  thread->affinity_mask = mask;

  uint32_t bound = thread->bound_core_id;
  if ((mask & (1U << bound)) == 0U) {
    for (uint32_t core = 0; core < g_active_core_count; ++core) {
      if ((mask & (1U << core)) != 0U) {
        return sched_migrate_tid(tid, core);
      }
    }
    return -1;
  }
  return 0;
}

int sched_sys_set_affinity(uint64_t tid, uint32_t affinity_mask) {
  return sched_set_affinity(tid, affinity_mask);
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

  uint32_t current_core = sched_clamp_core(hal_cpu_get_id());
  if (__atomic_load_n(&thread->owner_cpu, __ATOMIC_ACQUIRE) != current_core) {
      kernel_panic("sched_quarantine_thread: executing on non-owner CPU");
  }

  thread->state = THREAD_STATE_QUARANTINED;
  thread->owner_state = THREAD_OWNER_QUARANTINED;
  thread->pending_fault = (thread_fault_t)reason;

  // If enqueued locally, remove it
  if (thread->bound_core_id == current_core) {
      sched_rq_t *rq = sched_local_rq();
      thread_slot_t *slot = sched_find_thread_slot_by_tid_local(rq, thread->thread_id);
      if (slot && slot->is_on_runqueue) {
          hal_cpu_disable_interrupts();
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

int sched_quarantine_tid(uint64_t tid, uint32_t reason) {
  bh_thread_t *thread = sched_find_thread_by_id(tid);
  if (!thread) return -1;

  uint32_t current_core = sched_clamp_core(hal_cpu_get_id());
  uint32_t owner = __atomic_load_n(&thread->owner_cpu, __ATOMIC_ACQUIRE);

  if (owner != current_core) {
      sched_remote_cmd_t *cmd = sched_allocate_outbound_cmd();
      if (!cmd) return K_ERR_NO_RESOURCES;
      cmd->type = SCHED_REMOTE_QUARANTINE;
      cmd->source_cpu = current_core;
      cmd->target_cpu = owner;
      cmd->thread_id = thread->thread_id;
      cmd->expected_thread_generation = thread->sched_generation;
      cmd->flags = reason;
      cmd->state = SCHED_REMOTE_CMD_PENDING;

      kstatus_t status = sched_remote_submit(owner, cmd);
      if (status != K_OK) {
          sched_remote_cmd_release(cmd);
          return status;
      }
      return 0;
  }

  return sched_quarantine_thread(thread, reason);
}

int sched_request_handoff_tid(uint64_t tid, uint32_t target_cpu, uint32_t auth_token) {
  bh_thread_t *thread = sched_find_thread_by_id(tid);
  if (!thread || target_cpu >= g_active_core_count) {
    return -1;
  }

  uint32_t current_core = sched_clamp_core(hal_cpu_get_id());
  uint32_t owner = __atomic_load_n(&thread->owner_cpu, __ATOMIC_ACQUIRE);

  if (owner != current_core) {
    sched_remote_cmd_t *cmd = sched_allocate_outbound_cmd();
    if (!cmd) return K_ERR_NO_RESOURCES;
    cmd->type = SCHED_REMOTE_HANDOFF;
    cmd->source_cpu = current_core;
    cmd->target_cpu = target_cpu;
    cmd->thread_id = thread->thread_id;
    cmd->expected_thread_generation = thread->sched_generation;
    cmd->flags = auth_token;
    cmd->state = SCHED_REMOTE_CMD_PENDING;

    kstatus_t status = sched_remote_submit(owner, cmd);
    if (status != K_OK) {
        sched_remote_cmd_release(cmd);
        return status;
    }
    return 0;
  }

  if (target_cpu == current_core) {
    return -1;
  }

  thread_slot_t *slot = sched_find_thread_slot_by_tid(thread->thread_id);
  if (!slot) {
    return -1;
  }

  hal_cpu_disable_interrupts();

  if (thread->state != THREAD_STATE_READY) {
    hal_cpu_enable_interrupts();
    return -2;
  }

  sched_rq_t *rq = sched_local_rq();

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

  thread->state = THREAD_STATE_REMOTE_HANDOFF_PENDING;

  hal_cpu_enable_interrupts();

  mk_channel_t channel;
  if (mk_get_channel(current_core, target_cpu, &channel) != 0) {
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
    return -3;
  }

  mk_msg_thread_handoff_t payload = {
    .thread_id = thread->thread_id,
    .source_core = current_core,
    .target_core = target_cpu,
    .priority = thread->priority,
    .flags = 0
  };

  urpc_msg_t msg = {
    .type = MK_MSG_THREAD_HANDOFF_REQ,
    .payload_size = sizeof(payload),
    .src_core = current_core,
    .dst_core = target_cpu,
    .auth_token = auth_token
  };
  __builtin_memcpy(msg.payload_data, &payload, sizeof(payload));

  int ret = mk_send_message(&channel, msg.type, msg.payload_data, msg.payload_size);
  if (ret != 0) {
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
      return -4;
  }

  return 0;
}

