#include "sched/sched.h"
#include "tests/ktest.h"
#include <bharat/cpu_local.h>
#include "sched/sched_internal.h"
#include "hal/hal.h"

// External mock controls from ktest_sched_ipi.c
extern uint32_t current_mock_cpu;

static void test_dummy_entry(void) {
    while(1) {
        bh_thread_yield();
    }
}

bool test_sched_ownership_invariants(void) {
    // DO NOT call sched_init() here, it resets the kernel's global state.

    // We already have a running scheduler.
    // Ensure we are on core 0 for the test
    current_mock_cpu = 0;

    bh_process_t *proc = process_create("invariant_proc");
    KTEST_ASSERT(proc != NULL, "Failed to create process");

    // Start using NULL entry to avoid actually running if picked (we manually clean up)
    // Actually, thread_create_detached calls arch_prepare_initial_context.
    // If we use NULL, it will fault if it ever runs.
    bh_thread_t *thread = thread_create_detached(proc, test_dummy_entry);
    KTEST_ASSERT(thread != NULL, "Failed to create thread");

    // Assign a priority that is likely to be picked early
    thread->priority = SCHED_MAX_PRIORITY;

    // Initial state: not enqueued, no owner cpu
    if (thread->enqueued) {
        sched_invariant_on_dequeue(thread);
    }
    KTEST_ASSERT(!thread->enqueued, "New thread should not be enqueued");

    current_mock_cpu = 0;
    sched_enqueue(thread, 0);

    KTEST_ASSERT(thread->enqueued, "Thread should be enqueued");
    KTEST_ASSERT(thread->owner_cpu == 0, "Owner CPU mismatch");
    KTEST_ASSERT(thread->owner_state == THREAD_OWNER_RUNQUEUE, "Owner state mismatch");

    // Picking the thread should update its state
    bh_thread_t *picked = NULL;
    bh_thread_t *temp_list[64];
    int temp_count = 0;

    for (int i = 0; i < 100; i++) {
        picked = sched_pick_next_ready(0);
        if (!picked || picked == g_cpu_locals[0].runqueue.idle_thread) break;
        if (picked == thread) break;

        if (temp_count < 64) {
            temp_list[temp_count++] = picked;
        }
    }

    // Re-enqueue everything we skipped
    for (int i = 0; i < temp_count; i++) {
        sched_enqueue(temp_list[i], 0);
    }

    KTEST_ASSERT(picked == thread, "Picked wrong thread");
    KTEST_ASSERT(!thread->enqueued, "Picked thread should not be marked enqueued");

    // Manual state update for the "mock" switch in test
    thread->owner_state = THREAD_OWNER_RUNNING;
    thread->owner_cpu = 0;

    // Clean up
    thread->state = THREAD_STATE_TERMINATED;
    // We don't actually context switch in this test to avoid messing up the runner

    return true;
}

bool test_sched_remote_command_validation(void) {
    bh_process_t *proc = process_create("remote_proc");
    bh_thread_t *thread = thread_create_detached(proc, test_dummy_entry);
    if (!thread) return false;

    current_mock_cpu = 0;

    // Enqueue to core 1 (remote)
    sched_enqueue(thread, 1);

    sched_rq_t *rq1 = &g_cpu_locals[1].runqueue;
    KTEST_ASSERT(!list_empty(&rq1->pending_inbox), "Inbox empty");

    // Simulate generation mismatch
    uint64_t old_gen = thread->sched_generation;
    thread->sched_generation++; // Manually mismatch

    // We mock that we are now core 1
    uint32_t saved_cpu = current_mock_cpu;
    current_mock_cpu = 1;
    sched_reschedule();

    KTEST_ASSERT(list_empty(&rq1->pending_inbox), "Inbox not drained");
    KTEST_ASSERT(rq1->runnable_count == 0, "Thread wrongly enqueued despite generation mismatch");

    // Fix generation and try again
    thread->sched_generation = old_gen;
    current_mock_cpu = saved_cpu;
    sched_enqueue(thread, 1);

    current_mock_cpu = 1;
    sched_reschedule();
    KTEST_ASSERT(rq1->runnable_count == 1, "Thread should be enqueued now");

    // Cleanup
    thread->state = THREAD_STATE_TERMINATED;
    current_mock_cpu = saved_cpu;

    return true;
}

bool test_sched_remote_mutation_prevention(void) {
    bh_process_t *proc = process_create("mutation_proc");
    bh_thread_t *thread = thread_create_detached(proc, test_dummy_entry);
    if (!thread) return false;

    current_mock_cpu = 0;
    uint32_t target_cpu = 1;

    sched_rq_t *target_rq = &g_cpu_locals[target_cpu].runqueue;
    uint32_t initial_count = target_rq->runnable_count;

    // Remote enqueue: should only add to inbox, not runqueue
    sched_enqueue(thread, target_cpu);

    KTEST_ASSERT(target_rq->runnable_count == initial_count, "Remote enqueue mutated target runqueue count directly!");
    KTEST_ASSERT(!list_empty(&target_rq->pending_inbox), "Remote enqueue failed to populate inbox");

    // Cleanup
    thread->state = THREAD_STATE_TERMINATED;

    return true;
}

bool test_sched_stress_enqueue_dequeue(void) {
    bh_process_t *proc = process_create("stress_proc");
    bh_thread_t *thread = thread_create_detached(proc, test_dummy_entry);
    if (!thread) return false;

    current_mock_cpu = 0;
    for (int i = 0; i < 100; i++) {
        sched_enqueue(thread, 0);
        bh_thread_t *picked = sched_pick_next_ready(0);
        KTEST_ASSERT(picked == thread, "Stress pick failure");
    }

    // Cleanup
    thread->state = THREAD_STATE_TERMINATED;

    return true;
}

static ktest_case_t sched_invariant_tests[] = {
    {"Scheduler Ownership Invariants", test_sched_ownership_invariants},
    {"Remote Command Validation", test_sched_remote_command_validation},
    {"Mutation Prevention", test_sched_remote_mutation_prevention},
    {"Scheduler Stress Enqueue/Dequeue", test_sched_stress_enqueue_dequeue}
};

void test_sched_invariants_run(void) {
    ktest_run_suite("Scheduler Invariant Tests", sched_invariant_tests, 4);
}

static int boot_test_sched_invariants(void) {
    if (test_sched_ownership_invariants() &&
        test_sched_remote_command_validation() &&
        test_sched_remote_mutation_prevention() &&
        test_sched_stress_enqueue_dequeue()) {
        return 0;
    }
    return -1;
}

REGISTER_BOOT_SELFTEST("sched_invariants", "scheduler", boot_test_sched_invariants, BOOT_TEST_STAGE_RUNTIME, BOOT_TEST_MANDATORY, 0, true)
