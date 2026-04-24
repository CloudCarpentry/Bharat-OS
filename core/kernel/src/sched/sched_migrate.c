#include "sched/sched.h"
#include "sched_internal.h"

void sched_balance_once(void) {
  uint32_t busiest = 0U;
  uint32_t idlest = 0U;
  uint32_t max_depth = 0U;
  uint32_t min_depth = UINT32_MAX;

  for (uint32_t core = 0; core < MAX_SUPPORTED_CORES; ++core) {
    uint32_t depth = sched_run_queue_depth(core);
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

  bh_thread_t *candidate = sched_pick_next_ready(busiest);
  if (!candidate || candidate == g_cpu_locals[busiest].runqueue.idle_thread) {
    return;
  }

  if (!sched_is_core_admissible(candidate, idlest)) {
    (void)sched_enqueue(candidate, busiest);
    return;
  }

  if ((candidate->affinity_mask & (1U << idlest)) == 0U) {
    (void)sched_enqueue(candidate, busiest);
    return;
  }

  candidate->preferred_numa_node = (uint8_t)idlest;
  (void)sched_enqueue(candidate, idlest);
}

int sched_migrate_task(bh_thread_t *thread, uint32_t new_node) {
  if (!thread || new_node >= g_active_core_count) {
    return -1;
  }
  if ((thread->affinity_mask & (1U << new_node)) == 0U) {
    return -2;
  }

  thread_slot_t *slot = sched_find_thread_slot_by_tid_local(&g_cpu_locals[thread->home_core_id].runqueue, thread->thread_id);
  if (!slot) {
    return -1;
  }

  if (slot->is_on_runqueue != 0U) {
    uint32_t current_core = sched_clamp_core(hal_cpu_get_id());
    bool is_local = (thread->bound_core_id == current_core);

    if (is_local) {
        hal_cpu_disable_interrupts();
    } else {
        spin_lock(&g_cpu_locals[thread->bound_core_id].runqueue.lock);
    }

    sched_rq_t *rq = &g_cpu_locals[thread->bound_core_id].runqueue;

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

    if (is_local) {
        hal_cpu_enable_interrupts();
    } else {
        spin_unlock(&g_cpu_locals[thread->bound_core_id].runqueue.lock);
    }
  }

  thread->bound_core_id = new_node;
  thread->preferred_numa_node = (uint8_t)new_node;
  if (thread->state == THREAD_STATE_READY) {
    return sched_enqueue(thread, new_node); // Enqueues into target core's inbox if remote
  }
  return 0;
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

