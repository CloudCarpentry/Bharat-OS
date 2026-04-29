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
        if (thread->owner_state == THREAD_OWNER_NONE) {
            handle_violation(VIOLATION_RUNNABLE_NO_OWNER, thread, "Invariant failure: Runnable thread has no owner state");
        }
    }

    // Invariant: If enqueued, it must be on the runqueue of its owner_cpu.
    if (thread->enqueued) {
        if (thread->owner_state != THREAD_OWNER_RUNQUEUE) {
            handle_violation(VIOLATION_ENQUEUED_WRONG_OWNER, thread, "Invariant failure: Thread marked enqueued but owner_state is not RUNQUEUE");
        }
        if (thread->state != THREAD_STATE_READY) {
             handle_violation(VIOLATION_ENQUEUED_NOT_READY, thread, "Invariant failure: Enqueued thread not in READY state");
        }
    }

    // Invariant: Running thread must be marked as RUNNING owner state on its bound core.
    if (thread->state == THREAD_STATE_RUNNING) {
        if (thread->owner_state != THREAD_OWNER_RUNNING) {
            handle_violation(VIOLATION_RUNNING_WRONG_OWNER, thread, "Invariant failure: Running thread owner_state is not RUNNING");
        }
    }
}

void sched_invariant_on_enqueue(bh_thread_t *thread, uint32_t core_id) {
    if (thread->enqueued) {
        handle_violation(VIOLATION_DOUBLE_ENQUEUE, thread, "Invariant failure: Double enqueue detected");
    }
    if (thread->owner_state == THREAD_OWNER_RUNNING && thread->owner_cpu != core_id) {
        handle_violation(VIOLATION_RUNNING_ENQUEUE_NO_HANDOFF, thread, "Invariant failure: Enqueuing currently running thread to different core without handoff");
    }

    thread->enqueued = true;
    thread->owner_cpu = core_id;
    thread->owner_state = THREAD_OWNER_RUNQUEUE;
}

void sched_invariant_on_dequeue(bh_thread_t *thread) {
    if (!thread->enqueued) {
        handle_violation(VIOLATION_DEQUEUE_NOT_ENQUEUED, thread, "Invariant failure: Dequeue of non-enqueued thread");
    }
    thread->enqueued = false;
    // We keep owner_cpu but owner_state will be updated by the caller (e.g. to RUNNING or BLOCKED)
}

void sched_invariant_on_switch(bh_thread_t *prev, bh_thread_t *next, uint32_t core_id) {
    if (prev && prev->state == THREAD_STATE_RUNNING) {
        prev->owner_state = THREAD_OWNER_RUNNING;
        prev->owner_cpu = core_id;
    } else if (prev && prev->state == THREAD_STATE_BLOCKED) {
        prev->owner_state = THREAD_OWNER_BLOCKED;
    } else if (prev && prev->state == THREAD_STATE_SLEEPING) {
        prev->owner_state = THREAD_OWNER_BLOCKED; // Simplified for now
    }

    if (next) {
        if (next->enqueued) {
             handle_violation(VIOLATION_SWITCH_TO_ENQUEUED, next, "Invariant failure: Switching to thread still marked as enqueued");
        }
        next->owner_state = THREAD_OWNER_RUNNING;
        next->owner_cpu = core_id;
    }
}
