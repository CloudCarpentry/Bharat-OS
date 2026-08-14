#include "sched/sched.h"
#include "sched/sched_invariants.h"
#include "sched_internal.h"
#include "panic.h"

thread_slot_t *sched_find_thread_slot_by_tid_local(sched_rq_t *rq, uint64_t tid) {
  uint16_t home_core = bh_tid_home_core(tid);
  uint16_t slot_idx = bh_tid_slot(tid);
  if (home_core >= g_active_core_count || !rq || rq != &g_cpu_locals[home_core].runqueue) {
    return NULL;
  }
  if (slot_idx < SCHED_MAX_THREADS) {
    thread_slot_t *slots = (thread_slot_t *)rq->threads;
    if (slots && slots[slot_idx].in_use && slots[slot_idx].thread.thread_id == tid) {
      return &slots[slot_idx];
    }
  } else if (slot_idx >= SCHED_MAX_THREADS && slot_idx < SCHED_MAX_THREADS + 2U) {
    thread_slot_t *slots = (thread_slot_t *)rq->bootstrap_threads;
    uint32_t b_idx = slot_idx - SCHED_MAX_THREADS;
    if (slots && slots[b_idx].in_use && slots[b_idx].thread.thread_id == tid) {
      return &slots[b_idx];
    }
  }
  return NULL;
}

thread_slot_t *sched_find_thread_slot_by_tid(uint64_t tid) {
  uint16_t home_core = bh_tid_home_core(tid);
  if (home_core >= g_active_core_count) {
    return NULL;
  }
  sched_rq_t *rq = &g_cpu_locals[home_core].runqueue;
  return sched_find_thread_slot_by_tid_local(rq, tid);
}

thread_slot_t *sched_find_free_thread_slot(void) {
  uint32_t current_core = sched_clamp_core(hal_cpu_get_id());
  sched_rq_t *rq = &g_cpu_locals[current_core].runqueue;

  if (rq->free_thread_head == UINT32_MAX) {
    return NULL;
  }
  uint32_t idx = rq->free_thread_head;
  thread_slot_t *slots = (thread_slot_t *)rq->threads;
  rq->free_thread_head = slots[idx].next_free;
  return &slots[idx];
}

process_slot_t *sched_find_free_process_slot(void) {
  uint32_t current_core = sched_clamp_core(hal_cpu_get_id());
  sched_rq_t *rq = &g_cpu_locals[current_core].runqueue;

  if (rq->free_process_head == UINT32_MAX) {
    return NULL;
  }
  uint32_t idx = rq->free_process_head;
  process_slot_t *slots = (process_slot_t *)rq->processes;
  rq->free_process_head = slots[idx].next_free;
  return &slots[idx];
}

void sched_sleep_enqueue(thread_slot_t *slot, uint32_t core_id) {
  if (!slot || slot->is_sleeping != 0U || slot->is_blocked != 0U) {
    return;
  }
  list_add(&slot->wait_node, &g_cpu_locals[core_id].runqueue.sleeping_list);
  slot->is_sleeping = 1U;
}

void sched_sleep_dequeue(thread_slot_t *slot) {
  if (!slot || slot->is_sleeping == 0U) {
    return;
  }
  list_del(&slot->wait_node);
  list_init(&slot->wait_node);
  slot->is_sleeping = 0U;
}

void sched_block_enqueue(thread_slot_t *slot, uint32_t core_id) {
  if (!slot || slot->is_sleeping != 0U || slot->is_blocked != 0U) {
    return;
  }
  list_add(&slot->wait_node, &g_cpu_locals[core_id].runqueue.blocked_list);
  slot->is_blocked = 1U;
}

void sched_block_dequeue(thread_slot_t *slot) {
  if (!slot || slot->is_blocked == 0U) {
    return;
  }
  list_del(&slot->wait_node);
  list_init(&slot->wait_node);
  slot->is_blocked = 0U;
}

void sched_detach_thread_from_queues(thread_slot_t *slot) {
  if (!slot) {
    return;
  }
  bh_thread_t *thread = &slot->thread;
  uint32_t current_core = sched_clamp_core(hal_cpu_get_id());

  // Assert local ownership and local runqueue
  if (thread->owner_cpu != current_core &&
      thread->owner_state != THREAD_OWNER_REMOTE_PENDING) {
      kernel_panic("sched_detach_thread_from_queues failed: not local owner");
  }

  sched_rq_t *rq = sched_local_rq();
  sched_assert_local_rq(rq);

  hal_irq_state_t irq_state = hal_irq_save_disable();

  if (slot->is_on_runqueue != 0U) {
    if (rq->policy == SCHED_POLICY_CLOUD_FAIR) {
      sched_cfs_dequeue(rq, thread);
    } else if (rq->policy == SCHED_POLICY_EDF) {
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
    sched_validate_rq(rq);
  }
  if (slot->is_sleeping != 0U) {
    sched_sleep_dequeue(slot);
  }
  if (slot->is_blocked != 0U) {
    sched_block_dequeue(slot);
  }

  hal_irq_restore(irq_state);
}

int sched_enqueue_reap(thread_slot_t *slot) {
  if (!slot || slot->is_bootstrap != 0U) {
    return -1;
  }

  uint32_t core_id = sched_clamp_core(slot->creation_core_id);
  sched_rq_t *rq = &g_cpu_locals[core_id].runqueue;

  spin_lock(&rq->lock);
  if (slot->reap_pending == 0U) {
    slot->reap_pending = 1U;
    slot->reap_next = UINT32_MAX;
    uint32_t idx = (uint32_t)(slot - (thread_slot_t*)rq->threads);
    if (rq->reap_tail == UINT32_MAX) {
      rq->reap_head = idx;
      rq->reap_tail = idx;
    } else {
      ((thread_slot_t*)rq->threads)[rq->reap_tail].reap_next = idx;
      rq->reap_tail = idx;
    }
  }
  spin_unlock(&rq->lock);
  return 0;
}

void sched_reap_terminated_threads(void) {
  uint32_t current_core = sched_clamp_core(hal_cpu_get_id());
  sched_rq_t *rq = &g_cpu_locals[current_core].runqueue;

  while (1) {
    thread_slot_t *slot = NULL;

    spin_lock(&rq->lock);
    if (rq->reap_head != UINT32_MAX) {
      uint32_t idx = rq->reap_head;
      slot = &((thread_slot_t*)rq->threads)[idx];
      rq->reap_head = slot->reap_next;
      if (rq->reap_head == UINT32_MAX) {
        rq->reap_tail = UINT32_MAX;
      }
      slot->reap_next = UINT32_MAX;
      slot->reap_pending = 0U;
    }
    spin_unlock(&rq->lock);

    if (!slot) {
      break;
    }
    (void)thread_destroy(&slot->thread);
  }
}
