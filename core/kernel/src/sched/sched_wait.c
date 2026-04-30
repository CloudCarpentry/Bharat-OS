#include "sched/sched.h"
#include "sched_internal.h"

void sched_wait_queue_init(wait_queue_t* queue) {
  if (queue) {
    queue->head = NULL;
    queue->tail = NULL;
  }
}

void sched_wait_queue_enqueue(wait_queue_t* queue, bh_thread_t* thread) {
  if (!queue || !thread) {
    return;
  }

  thread->next_waiter = NULL;

  if (!queue->tail) {
    queue->head = thread;
    queue->tail = thread;
  } else {
    queue->tail->next_waiter = thread;
    queue->tail = thread;
  }
}

bh_thread_t* sched_wait_queue_dequeue(wait_queue_t* queue) {
  if (!queue || !queue->head) {
    return NULL;
  }

  bh_thread_t* thread = queue->head;
  // Skip any threads that were already woken up by timeout (state != BLOCKED)
  // or that had their next_waiter cleared by the timeout sweep.
  while (thread && thread->state != THREAD_STATE_BLOCKED) {
      thread = thread->next_waiter;
      queue->head = thread;
  }

  if (!queue->head) {
    queue->tail = NULL;
    return NULL;
  }

  thread = queue->head;
  queue->head = thread->next_waiter;
  if (!queue->head) {
    queue->tail = NULL;
  }

  thread->next_waiter = NULL;
  return thread;
}

void sched_block(void) {
  uint32_t core = sched_clamp_core(hal_cpu_get_id());
  bh_thread_t *current = g_cpu_locals[core].runqueue.current_thread;
  if (current) {
    current->state = THREAD_STATE_BLOCKED;
    if (current->sched_ctx && current->sched_ctx->deg) {
        deg_block_member(current, 0);
    }

    if (current->ipc_deadline_ticks > 0) {
      thread_slot_t *slot = sched_find_thread_slot_by_tid_local(&g_cpu_locals[core].runqueue, current->thread_id);
      if (slot) {
        sched_block_enqueue(slot, core);
      }
    }

    g_cpu_locals[core].runqueue.current_thread = NULL;
  }
}

void sched_sleep(uint64_t millis) {
  uint32_t core = sched_clamp_core(hal_cpu_get_id());
  bh_thread_t *current = g_cpu_locals[core].runqueue.current_thread;
  if (!current || current == g_cpu_locals[core].runqueue.idle_thread) {
    return;
  }

  thread_slot_t *slot = sched_find_thread_slot_by_tid_local(&g_cpu_locals[core].runqueue, current->thread_id);
  if (!slot) {
    return;
  }

  current->wake_deadline_ms = g_cpu_locals[core].runqueue.total_ticks + millis;
  current->state = THREAD_STATE_SLEEPING;
  sched_sleep_enqueue(slot, core);
  g_cpu_locals[core].runqueue.current_thread = NULL;
  sched_reschedule();
}

void sched_wakeup_with_priority(bh_thread_t *thread, uint32_t wakeup_priority) {
  if (!thread) {
    return;
  }

  uint32_t current_core = sched_clamp_core(hal_cpu_get_id());
  if (thread->home_core_id != current_core) {
      sched_rq_t *target_rq = &g_cpu_locals[thread->home_core_id].runqueue;
      thread_slot_t *slot = sched_find_thread_slot_by_tid_local(&g_cpu_locals[thread->home_core_id].runqueue, thread->thread_id);
      if (!slot) return;

      spin_lock(&target_rq->lock);

      sched_remote_cmd_t *cmd = &slot->remote_cmd;
      cmd->type = SCHED_REMOTE_WAKE;
      cmd->thread_ref.thread = thread;
      cmd->thread_ref.thread_id = (uint32_t)thread->thread_id;
      cmd->thread_ref.generation = thread->sched_generation;
      cmd->thread_ref.source_cpu = current_core;
      cmd->thread_ref.target_cpu = thread->home_core_id;
      cmd->priority = wakeup_priority;

      list_add_tail(&cmd->list, &target_rq->pending_inbox);

      if (target_rq->resched_pending == 0) {
          target_rq->resched_pending = 1;
          target_rq->ipi_sent++;
          spin_unlock(&target_rq->lock);
          hal_send_ipi_payload(1U << thread->home_core_id, MK_MSG_THREAD_WAKE_REQ);
      } else {
          target_rq->ipi_coalesced++;
          spin_unlock(&target_rq->lock);
      }
      return;
  }

  if (wakeup_priority <= SCHED_MAX_PRIORITY && wakeup_priority > thread->priority) {
    thread->priority = wakeup_priority;
  }

  thread_slot_t *slot = sched_find_thread_slot_by_tid_local(&g_cpu_locals[thread->home_core_id].runqueue, thread->thread_id);
  if (!slot) {
    return;
  }

  if (thread->state == THREAD_STATE_SLEEPING || thread->state == THREAD_STATE_BLOCKED) {
    thread->state = THREAD_STATE_READY;
    thread->wake_deadline_ms = 0U;
    if (slot->is_sleeping != 0U) {
      sched_sleep_dequeue(slot);
    }
    if (slot->is_blocked != 0U) {
      sched_block_dequeue(slot);
    }
    (void)sched_enqueue(thread, thread->bound_core_id);
  }
}

void sched_wakeup(bh_thread_t *thread) {
  sched_wakeup_with_priority(thread, SCHED_MAX_PRIORITY + 1U);
}

