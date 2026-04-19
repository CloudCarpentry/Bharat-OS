#include "sched/sched.h"
#include "sched_internal.h"
#include "panic.h"

int sched_enqueue(kthread_t *thread, uint32_t core_id) {
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
      mk_msg_remote_lookup_t req = {
          .request_id = 0,
          .src_core = current_core,
          .target_core = core_id,
          .id = thread->thread_id,
          .arg = core_id
      };
      mk_channel_t *chan = NULL;
      if (mk_get_channel(current_core, core_id, chan) == 0 && chan) {
          mk_send_message(chan, MK_MSG_THREAD_ENQUEUE_REQ, &req, sizeof(req));
          hal_send_ipi_payload(1U << core_id, MK_MSG_THREAD_ENQUEUE_REQ);
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

  if (g_policy == SCHED_POLICY_CLOUD_FAIR) {
    sched_cfs_enqueue(rq, thread);
  } else if (g_policy == SCHED_POLICY_EDF) {
    if (thread->rt_attr.period_ms > 0 && thread->rt_attr.deadline_ms > 0) {
        if (thread->absolute_deadline_ms == 0) {
            thread->absolute_deadline_ms = g_cpu_locals[sched_clamp_core(hal_cpu_get_id())].runqueue.total_ticks + thread->rt_attr.deadline_ms;
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

static void sched_dequeue_task_l0(kthread_t *thread, uint32_t core_id) {
  (void)core_id;
  if (!thread) {
    return;
  }
  sched_rq_t *rq = &g_cpu_locals[core_id].runqueue;
  thread_slot_t *slot = sched_find_thread_slot_by_tid_local(rq, thread->thread_id);
  if (slot && slot->is_on_runqueue != 0U) {
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

void sched_enqueue_task_l0(kthread_t *thread, uint32_t core_id) {
  (void)sched_enqueue(thread, core_id);
}

void sched_enqueue_task_l1(kthread_t *thread, uint32_t core_id) {
  (void)sched_enqueue(thread, core_id);
}

void sched_dequeue_task_l1(kthread_t *thread, uint32_t core_id) {
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
            kthread_t *next = (kthread_t *)(void *)((char *)first - offsetof(kthread_t, cfs_node));
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
