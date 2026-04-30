#include "sched/sched.h"
#include "sched/sched_invariants.h"
#include "sched_internal.h"
#include "panic.h"
#include "hal/hal.h"

typedef enum {
    SCHED_FAULT_ACTION_PANIC,
    SCHED_FAULT_ACTION_QUARANTINE_THREAD,
    SCHED_FAULT_ACTION_ISOLATE_CORE,
    SCHED_FAULT_ACTION_RETURN_ERROR
} sched_fault_action_t;

typedef enum {
    VIOLATION_RUNNABLE_NO_OWNER,
    VIOLATION_ENQUEUED_WRONG_OWNER,
    VIOLATION_ENQUEUED_NOT_READY,
    VIOLATION_RUNNING_WRONG_OWNER,
    VIOLATION_DOUBLE_ENQUEUE,
    VIOLATION_RUNNING_ENQUEUE_NO_HANDOFF,
    VIOLATION_DEQUEUE_NOT_ENQUEUED,
    VIOLATION_SWITCH_TO_ENQUEUED
} sched_invariant_violation_t;

static sched_fault_action_t sched_fault_policy_for_invariant(sched_invariant_violation_t violation) {
#if defined(BHARAT_KERNEL_HARDENING_FATAL)
    return SCHED_FAULT_ACTION_PANIC;
#elif defined(BHARAT_PROFILE_RTOS) || defined(BHARAT_PROFILE_SAFETY)
    (void)violation;
    return SCHED_FAULT_ACTION_PANIC; // For now keep it fatal for safety
#elif defined(BHARAT_HOST_TEST)
    return SCHED_FAULT_ACTION_RETURN_ERROR;
#else
    return SCHED_FAULT_ACTION_QUARANTINE_THREAD;
#endif
}

static void handle_violation(sched_invariant_violation_t violation, bh_thread_t *thread, const char *msg) {
    sched_fault_action_t action = sched_fault_policy_for_invariant(violation);
    switch (action) {
        case SCHED_FAULT_ACTION_PANIC:
            kernel_panic(msg);
            break;
        case SCHED_FAULT_ACTION_QUARANTINE_THREAD:
            if (thread) {
                sched_quarantine_thread(thread, (uint32_t)violation);
            }
            break;
        case SCHED_FAULT_ACTION_ISOLATE_CORE:
            // Minimal core isolation: just set a flag on this core for now
            g_cpu_locals[hal_cpu_get_id()].runqueue.sched_isolated = true;
            g_cpu_locals[hal_cpu_get_id()].runqueue.isolation_reason = (uint32_t)violation;
            break;
        default:
            break;
    }
}

void sched_invariant_check_thread(bh_thread_t *thread) {
    if (!thread) return;

    // Invariant: If thread is runnable, it must have an owner CPU.
    if (thread->state == THREAD_STATE_READY || thread->state == THREAD_STATE_RUNNING) {
        if (thread->sched_queue_state == SCHED_QUEUE_NOT_QUEUED && thread->state == THREAD_STATE_READY) {
            handle_violation(VIOLATION_RUNNABLE_NO_OWNER, thread, "Invariant failure: Ready thread not queued");
        }
    }

    // Invariant: If enqueued, it must be on the runqueue of its sched_owner_cpu.
    if (thread->enqueued) {
        if (thread->sched_queue_state != SCHED_QUEUE_QUEUED) {
            handle_violation(VIOLATION_ENQUEUED_WRONG_OWNER, thread, "Invariant failure: Thread marked enqueued but sched_queue_state is not QUEUED");
        }
        if (thread->state != THREAD_STATE_READY) {
             handle_violation(VIOLATION_ENQUEUED_NOT_READY, thread, "Invariant failure: Enqueued thread not in READY state");
        }
    }

    // Invariant: Running thread must be marked as RUNNING sched_queue_state on its bound core.
    if (thread->state == THREAD_STATE_RUNNING) {
        if (thread->sched_queue_state != SCHED_QUEUE_RUNNING) {
            handle_violation(VIOLATION_RUNNING_WRONG_OWNER, thread, "Invariant failure: Running thread sched_queue_state is not RUNNING");
        }
    }

    // Invariant: One runnable entity has exactly one owning CPU/queue
}

void sched_invariant_on_enqueue(bh_thread_t *thread, uint32_t core_id) {
    if (thread->enqueued) {
        handle_violation(VIOLATION_DOUBLE_ENQUEUE, thread, "Invariant failure: Double enqueue detected");
    }
    if (thread->sched_queue_state == SCHED_QUEUE_RUNNING && thread->sched_owner_cpu != core_id) {
        handle_violation(VIOLATION_RUNNING_ENQUEUE_NO_HANDOFF, thread, "Invariant failure: Enqueuing currently running thread to different core without handoff");
    }

    thread->enqueued = true;
    thread->sched_owner_cpu = core_id;
    thread->sched_queue_state = SCHED_QUEUE_QUEUED;
}

void sched_invariant_on_dequeue(bh_thread_t *thread) {
    if (!thread->enqueued) {
        handle_violation(VIOLATION_DEQUEUE_NOT_ENQUEUED, thread, "Invariant failure: Dequeue of non-enqueued thread");
    }
    thread->enqueued = false;
}

void sched_invariant_on_switch(bh_thread_t *prev, bh_thread_t *next, uint32_t core_id) {
    if (prev) {
        if (prev->state == THREAD_STATE_RUNNING) {
            prev->sched_queue_state = SCHED_QUEUE_RUNNING;
            prev->sched_owner_cpu = core_id;
        } else if (prev->state == THREAD_STATE_BLOCKED || prev->state == THREAD_STATE_SLEEPING) {
            prev->sched_queue_state = SCHED_QUEUE_BLOCKED;
        } else if (prev->state == THREAD_STATE_TERMINATED) {
            prev->sched_queue_state = SCHED_QUEUE_DYING;
        }
    }

    if (next) {
        if (next->enqueued) {
             handle_violation(VIOLATION_SWITCH_TO_ENQUEUED, next, "Invariant failure: Switching to thread still marked as enqueued");
        }
        next->sched_queue_state = SCHED_QUEUE_RUNNING;
        next->sched_owner_cpu = core_id;
    }
}

void sched_invariant_check_thread_owner(bh_thread_t *thread, uint32_t expected_cpu) {
    if (thread->sched_owner_cpu != expected_cpu) {
        kernel_panic("Invariant failure: Thread owner CPU mismatch");
    }
}

void sched_invariant_check_runqueue_exclusive(bh_thread_t *thread) {
    if (thread->enqueued && thread->sched_queue_state != SCHED_QUEUE_QUEUED) {
         kernel_panic("Invariant failure: Thread runqueue exclusivity violated");
    }
}

void sched_invariant_check_remote_enqueue_path(bh_thread_t *thread) {
    if (thread->sched_queue_state != SCHED_QUEUE_MIGRATING &&
        thread->sched_queue_state != SCHED_QUEUE_NOT_QUEUED) {
        kernel_panic("Invariant failure: Remote enqueue from invalid state");
    }
}
