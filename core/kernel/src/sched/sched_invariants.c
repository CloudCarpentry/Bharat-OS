#include "sched/sched.h"
#include "sched/sched_invariants.h"
#include "sched_internal.h"
#include "panic.h"
#include "hal/hal.h"

void sched_invariant_check_thread(bh_thread_t *thread) {
    if (!thread) return;

    // Invariant: If thread is runnable, it must have an owner CPU.
    if (thread->state == THREAD_STATE_READY || thread->state == THREAD_STATE_RUNNING) {
        if (thread->owner_state == THREAD_OWNER_NONE) {
            kernel_panic("Invariant failure: Runnable thread has no owner state");
        }
    }

    // Invariant: If enqueued, it must be on the runqueue of its owner_cpu.
    if (thread->enqueued) {
        if (thread->owner_state != THREAD_OWNER_RUNQUEUE) {
            kernel_panic("Invariant failure: Thread marked enqueued but owner_state is not RUNQUEUE");
        }
        if (thread->state != THREAD_STATE_READY) {
             kernel_panic("Invariant failure: Enqueued thread not in READY state");
        }
    }

    // Invariant: Running thread must be marked as RUNNING owner state on its bound core.
    if (thread->state == THREAD_STATE_RUNNING) {
        if (thread->owner_state != THREAD_OWNER_RUNNING) {
            kernel_panic("Invariant failure: Running thread owner_state is not RUNNING");
        }
    }
}

void sched_invariant_on_enqueue(bh_thread_t *thread, uint32_t core_id) {
    if (thread->enqueued) {
        kernel_panic("Invariant failure: Double enqueue detected");
    }
    if (thread->owner_state == THREAD_OWNER_RUNNING && thread->owner_cpu != core_id) {
        kernel_panic("Invariant failure: Enqueuing currently running thread to different core without handoff");
    }

    thread->enqueued = true;
    thread->owner_cpu = core_id;
    thread->owner_state = THREAD_OWNER_RUNQUEUE;
}

void sched_invariant_on_dequeue(bh_thread_t *thread) {
    if (!thread->enqueued) {
        kernel_panic("Invariant failure: Dequeue of non-enqueued thread");
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
             kernel_panic("Invariant failure: Switching to thread still marked as enqueued");
        }
        next->owner_state = THREAD_OWNER_RUNNING;
        next->owner_cpu = core_id;
    }
}
