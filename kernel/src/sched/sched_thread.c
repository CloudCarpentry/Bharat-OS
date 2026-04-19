#include "sched/sched.h"
#include "sched_internal.h"

int process_destroy(kprocess_t *process) {
  if (!process) {
    return -1;
  }

  uint32_t current_core = sched_clamp_core(process->owner_core_id);
  sched_rq_t *rq = &g_cpu_locals[current_core].runqueue;

  process_slot_t *slot = NULL;
  for (size_t i = 0; i < SCHED_MAX_PROCESSES; ++i) {
    process_slot_t *s = &((process_slot_t*)rq->processes)[i];
    if (s->in_use != 0U && &s->process == process) {
      slot = s;
      break;
    }
  }

  if (!slot) {
    return -1;
  }

  for (size_t i = 0; i < SCHED_MAX_THREADS; ++i) {
    thread_slot_t *ts = &((thread_slot_t*)rq->threads)[i];
    if (ts->in_use != 0U && ts->thread.process_id == slot->process.process_id) {
      return -1;
    }
  }

  if (slot->process.addr_space) {
    (void)aspace_destroy(slot->process.addr_space);
    slot->process.addr_space = NULL;
  }

  if (slot->process.security_sandbox_ctx) {
    cap_table_destroy(slot->process.security_sandbox_ctx);
    slot->process.security_sandbox_ctx = NULL;
  }

  slot->in_use = 0U;
  uint32_t idx = (uint32_t)(slot - (process_slot_t*)rq->processes);
  slot->next_free = rq->free_process_head;
  rq->free_process_head = idx;
  return 0;
}

int thread_destroy(kthread_t *thread) {
  // Reaper-only contract:
  // - call only from deferred reap context, never inline on the running thread.
  // - never destroy while executing on the victim thread's stack.
  if (!thread) {
    return -1;
  }
  if (thread == sched_current_thread()) {
    return -1;
  }

  uint32_t creation_core = sched_clamp_core(thread->home_core_id);
  sched_rq_t *rq = &g_cpu_locals[creation_core].runqueue;
  thread_slot_t *slot = sched_find_thread_slot_by_tid_local(rq, thread->thread_id);
  if (!slot) {
    return -1;
  }

  // Prevent race with background reaper or other cores
  hal_cpu_disable_interrupts();
  
  spin_lock(&rq->lock);
  
  if (slot->reap_pending && slot->reap_next != UINT32_MAX) {
      // Thread is already being reaped, let the reaper finish it
      spin_unlock(&rq->lock);
      hal_cpu_enable_interrupts();
      return 0;
  }
  
  slot->reap_pending = 1U; // Mark as pending so reaper doesn't touch it
  spin_unlock(&rq->lock);

  sched_detach_thread_from_queues(slot);

  // Clean up architecture extended state
  arch_ext_state_thread_destroy(thread);

  if (thread->capability_list) {
    cap_table_destroy(thread->capability_list);
    thread->capability_list = NULL;
  }

  if (thread->kernel_stack) {
    void *stack_ptr = (void*)(uintptr_t)thread->kernel_stack;
    thread->kernel_stack = 0U; // Clear first to prevent double-free if re-entered
    kfree(stack_ptr);
  }

  // Final slot reclamation
  spin_lock(&rq->lock);
  slot->in_use = 0U;
  slot->reap_pending = 0U;

  if (!slot->is_bootstrap) {
    uint32_t idx = (uint32_t)(slot - (thread_slot_t*)rq->threads);
    slot->next_free = rq->free_thread_head;
    rq->free_thread_head = idx;
  }
  spin_unlock(&rq->lock);

  hal_cpu_enable_interrupts();
  return 0;
}

int sched_sys_thread_create(kprocess_t *parent, void (*entry_point)(void), uint64_t *out_tid) {
  kthread_t *t = thread_create(parent, entry_point);
  if (!t) {
    return -1;
  }
  if (out_tid) {
    *out_tid = t->thread_id;
  }
  return 0;
}

int sched_sys_thread_destroy(uint64_t tid) {
  thread_slot_t *slot = sched_resolve_tid_owner_slow(tid);
  if (!slot) {
    return -1;
  }
  return sched_mark_thread_terminated(&slot->thread);
}

int thread_raise_fault(kthread_t *thread, thread_fault_t fault) {
    if (!thread) return -1; // -EINVAL mapped

    thread->pending_fault = fault;
    thread->fault_pending = true;
    thread->state = THREAD_STATE_TERMINATED; // Mark as doomed

    /* TODO(personality/linux): translate THREAD_FAULT_SEGV / STACK_OVERFLOW to SIGSEGV */

    sched_reschedule();
    return 0;
}

