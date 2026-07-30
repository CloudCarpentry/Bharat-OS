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
  sched_rq_t *rq = sched_local_rq();
  bh_thread_t *current = rq->current_thread;
  if (current) {
    current->state = THREAD_STATE_BLOCKED;
    if (current->sched_ctx && current->sched_ctx->deg) {
        deg_block_member(current, 0);
    }

    if (current->ipc_deadline_ticks > 0) {
      thread_slot_t *slot = sched_find_thread_slot_by_tid_local(rq, current->thread_id);
      if (slot) {
        sched_block_enqueue(slot, core);
      }
    }

    rq->current_thread = NULL;
  }
}

void sched_sleep(uint64_t millis) {
  uint32_t core = sched_clamp_core(hal_cpu_get_id());
  sched_rq_t *rq = sched_local_rq();
  bh_thread_t *current = rq->current_thread;
  if (!current || current == rq->idle_thread) {
    return;
  }

  thread_slot_t *slot = sched_find_thread_slot_by_tid_local(rq, current->thread_id);
  if (!slot) {
    return;
  }

  current->wake_deadline_ms = rq->total_ticks + millis;
  current->state = THREAD_STATE_SLEEPING;
  sched_sleep_enqueue(slot, core);
  rq->current_thread = NULL;
  sched_reschedule();
}

int sched_wake_tid_with_priority(uint64_t tid, uint32_t priority) {
  bh_thread_t *thread = sched_find_thread_by_id(tid);
  if (!thread) return -1;

  uint32_t current_core = sched_clamp_core(hal_cpu_get_id());
  uint32_t owner = __atomic_load_n(&thread->owner_cpu, __ATOMIC_ACQUIRE);

  if (owner != current_core) {
      // Remote owner. Route command to owner_cpu.
      sched_remote_cmd_t *cmd = sched_allocate_outbound_cmd();
      if (!cmd) {
          return K_ERR_NO_RESOURCES;
      }

      cmd->type = SCHED_REMOTE_WAKE;
      cmd->source_cpu = current_core;
      cmd->target_cpu = owner;
      cmd->thread_id = thread->thread_id;
      cmd->expected_thread_generation = thread->sched_generation;
      cmd->priority = priority;
      cmd->state = SCHED_REMOTE_CMD_PENDING;

      kstatus_t status = sched_remote_submit(owner, cmd);
      if (status != K_OK) {
          sched_remote_cmd_release(cmd);
          return status;
      }
      return 0;
  }

  // Local wakeup
  if (priority <= SCHED_MAX_PRIORITY && priority > thread->priority) {
    thread->priority = priority;
  }

  thread_slot_t *slot = sched_find_thread_slot_by_tid(thread->thread_id);
  if (!slot) {
    return -1;
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
  return 0;
}

int sched_wake_tid(uint64_t tid) {
  return sched_wake_tid_with_priority(tid, SCHED_MAX_PRIORITY + 1U);
}

void sched_wakeup_with_priority(bh_thread_t *thread, uint32_t wakeup_priority) {
  if (thread) {
    sched_wake_tid_with_priority(thread->thread_id, wakeup_priority);
  }
}

void sched_wakeup(bh_thread_t *thread) {
  if (thread) {
    sched_wake_tid(thread->thread_id);
  }
}

