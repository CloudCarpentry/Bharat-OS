#include "sched/sched.h"
#include "sched/sched_invariants.h"
#include "sched_internal.h"
#include "panic.h"

int sched_enqueue(bh_thread_t *thread, uint32_t core_id) {
  if (!thread || thread->priority >= MAX_PRIORITY_LEVELS) {
    return -1;
  }

  core_id = sched_clamp_core(core_id);
  if (!sched_is_core_admissible(thread, core_id)) {
    return -1; // SCHED_REJECT
  }
  uint32_t current_core = sched_clamp_core(hal_cpu_get_id());
  bool is_local = (core_id == current_core);

  if (!is_local) {
      sched_rq_t *target_rq = &g_cpu_locals[core_id].runqueue;

      if (target_rq->sched_isolated) {
          return K_ERR_ISOLATED;
      }

      thread_slot_t *slot = sched_find_thread_slot_by_tid_local(&g_cpu_locals[thread->home_core_id].runqueue, thread->thread_id);
      if (!slot) return -1;

      sched_invariant_check_remote_enqueue_path(thread);

      spin_lock(&target_rq->lock);

      target_rq->remote_enqueues++;

      sched_remote_cmd_t *cmd = &slot->remote_cmd;
      cmd->type = SCHED_REMOTE_ENQUEUE;
      cmd->source_cpu = current_core;
      cmd->target_cpu = core_id;
      cmd->thread_id = thread->thread_id;
      cmd->expected_thread_generation = thread->sched_generation;
      cmd->priority = thread->priority;

      list_add_tail(&cmd->list, &target_rq->pending_inbox);

      if (target_rq->resched_pending == 0) {
          target_rq->resched_pending = 1;
          target_rq->ipi_sent++;
          spin_unlock(&target_rq->lock);
          hal_send_ipi_payload(1U << core_id, MK_MSG_THREAD_ENQUEUE_REQ);
      } else {
          target_rq->ipi_coalesced++;
          spin_unlock(&target_rq->lock);
      }
      return 0;
  }

  sched_rq_t *rq = &g_cpu_locals[core_id].runqueue;
  thread_slot_t *slot = sched_find_thread_slot_by_tid_local(rq, thread->thread_id);
  if (!slot) {
    return -1;
  }

  hal_cpu_disable_interrupts();

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

  thread->bound_core_id = core_id;
  thread->state = THREAD_STATE_READY;

  sched_invariant_on_enqueue(thread, core_id);

  if (g_policy == SCHED_POLICY_CLOUD_FAIR) {
    sched_cfs_enqueue(rq, thread);
  } else if (g_policy == SCHED_POLICY_EDF) {
    if (thread->rt_attr.period_ms > 0 && thread->rt_attr.deadline_ms > 0) {
        if (thread->absolute_deadline_ms == 0) {
            thread->absolute_deadline_ms = g_cpu_locals[current_core].runqueue.total_ticks + thread->rt_attr.deadline_ms;
        }
    }
    sched_edf_enqueue(rq, thread);
  } else {
    list_add(&slot->run_node, &rq->ready_queue[thread->priority]);
    sched_ready_bitmap_set(rq, thread->priority);
  }

  slot->is_on_runqueue = 1U;
  rq->runnable_count++;

  sched_validate_rq(rq);

  hal_cpu_enable_interrupts();
  return 0;
}

uint32_t sched_run_queue_depth(uint32_t core_id) {
  return g_cpu_locals[core_id].runqueue.runnable_count;
}

void sched_ready_bitmap_set(sched_rq_t *rq, uint32_t prio) {
  if (!rq || prio >= MAX_PRIORITY_LEVELS) {
    return;
  }
  rq->ready_bitmap |= (1U << prio);
}

void sched_ready_bitmap_clear_if_empty(sched_rq_t *rq, uint32_t prio) {
  if (!rq || prio >= MAX_PRIORITY_LEVELS) {
    return;
  }
  if (list_empty(&rq->ready_queue[prio])) {
    rq->ready_bitmap &= ~(1U << prio);
  }
}

int sched_pick_priority_from_bitmap(const sched_rq_t *rq, int highest) {
  if (!rq || rq->ready_bitmap == 0U) {
    return -1;
  }
  if (highest != 0) {
    return 31 - __builtin_clz(rq->ready_bitmap);
  }
  return __builtin_ctz(rq->ready_bitmap);
}

static void sched_dequeue_task_l0(bh_thread_t *thread, uint32_t core_id) {
  if (!thread) {
    return;
  }
  sched_rq_t *rq = &g_cpu_locals[core_id].runqueue;
  thread_slot_t *slot = sched_find_thread_slot_by_tid_local(rq, thread->thread_id);
  if (slot && slot->is_on_runqueue != 0U) {
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
}

void sched_enqueue_task_l0(bh_thread_t *thread, uint32_t core_id) {
  (void)sched_enqueue(thread, core_id);
}

void sched_enqueue_task_l1(bh_thread_t *thread, uint32_t core_id) {
  (void)sched_enqueue(thread, core_id);
}

void sched_dequeue_task_l1(bh_thread_t *thread, uint32_t core_id) {
  sched_dequeue_task_l0(thread, core_id);
}

void sched_validate_rq(sched_rq_t *rq) {
    // Debug only
#ifndef NDEBUG
    if (!rq) return;

    // Valid count
    if (rq->runnable_count > SCHED_MAX_THREADS) {
        kernel_panic("Runqueue count invalid/underflow");
    }

    if (g_policy == SCHED_POLICY_CLOUD_FAIR) {
        // Validate min_vruntime is sensible
        struct rb_node *first = rb_first(&rq->cfs_runqueue);
        if (first) {
            bh_thread_t *next = (bh_thread_t *)(void *)((char *)first - offsetof(bh_thread_t, cfs_node));
            if (next->vruntime < rq->min_vruntime && rq->min_vruntime - next->vruntime > 1000) {
                // Minor drift is okay due to rounding, but major divergence is a bug
                kernel_panic("Runqueue min_vruntime divergence");
            }
        }
    } else {
        // Validate priority bitmaps vs lists
        for (uint32_t i = 0; i < MAX_PRIORITY_LEVELS; i++) {
            int is_empty = list_empty(&rq->ready_queue[i]);
            int bit_set = (rq->ready_bitmap & (1U << i)) != 0;
            if (is_empty && bit_set) {
                 kernel_panic("Runqueue bitmap indicates ready task but list is empty");
            } else if (!is_empty && !bit_set) {
                 kernel_panic("Runqueue list has tasks but bitmap bit is cleared");
            }
        }
    }
#else
    (void)rq;
#endif
}
