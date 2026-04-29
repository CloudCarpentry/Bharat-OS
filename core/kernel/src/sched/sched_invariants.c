#include "sched/sched.h"
#include "sched/sched_invariants.h"
#include "sched_internal.h"
#include "panic.h"
#include "hal/hal.h"

void sched_invariant_check_thread(bh_thread_t *thread) {
    if (!thread) return;

    // Invariant: If thread is runnable, it must have an owner CPU.
    if (thread->state == THREAD_STATE_READY || thread->state == THREAD_STATE_RUNNING) {
        if (thread->sched_queue_state == SCHED_QUEUE_NOT_QUEUED && thread->state == THREAD_STATE_READY) {
            kernel_panic("Invariant failure: Ready thread not queued");
        }
    }

    // Invariant: If enqueued, it must be on the runqueue of its sched_owner_cpu.
    if (thread->enqueued) {
        if (thread->sched_queue_state != SCHED_QUEUE_QUEUED) {
            kernel_panic("Invariant failure: Thread marked enqueued but sched_queue_state is not QUEUED");
        }
    }

    // Invariant: Running thread must be marked as RUNNING sched_queue_state on its bound core.
    if (thread->state == THREAD_STATE_RUNNING) {
        if (thread->sched_queue_state != SCHED_QUEUE_RUNNING) {
            kernel_panic("Invariant failure: Running thread sched_queue_state is not RUNNING");
        }
    }

    // Invariant: One runnable entity has exactly one owning CPU/queue
}

void sched_invariant_on_enqueue(bh_thread_t *thread, uint32_t core_id) {
    if (thread->enqueued) {
        kernel_panic("Invariant failure: Double enqueue detected");
    }
    if (thread->sched_queue_state == SCHED_QUEUE_RUNNING && thread->sched_owner_cpu != core_id) {
        kernel_panic("Invariant failure: Enqueuing currently running thread to different core without handoff");
    }

    thread->enqueued = true;
    thread->sched_owner_cpu = core_id;
    thread->sched_queue_state = SCHED_QUEUE_QUEUED;
}

void sched_invariant_on_dequeue(bh_thread_t *thread) {
    if (!thread->enqueued) {
        kernel_panic("Invariant failure: Dequeue of non-enqueued thread");
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
             kernel_panic("Invariant failure: Switching to thread still marked as enqueued");
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
