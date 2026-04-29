#include "sched/sched.h"
#include "tests/ktest.h"
#include "hal/hal.h"
#include <bharat/cpu_local.h>

extern void sched_init(void);

// Provide a mock cpu id since we run on a uniprocessor host test runner
uint32_t current_mock_cpu = 0;

#if defined(BHARAT_HOST_TEST) || !defined(__KERNEL__)
uint32_t hal_cpu_get_id(void) {
    return current_mock_cpu;
}

uint32_t mock_ipi_sent_to = UINT32_MAX;

void hal_send_ipi_payload(uint32_t target_core, uint64_t payload) {
    (void)payload;
    mock_ipi_sent_to = target_core;
}
#else
// In kernel builds, these are provided by the arch HAL.
// We use weak stubs or just rely on the HAL.
extern uint32_t hal_cpu_get_id(void);
extern void hal_send_ipi_payload(uint32_t target_core, uint64_t payload);
uint32_t mock_ipi_sent_to = UINT32_MAX;
#endif

static void dummy_thread_entry(void) {
    while(1) {}
}

static bool sched_test_multicore_available(void) {
#if defined(BHARAT_HOST_TEST) || !defined(__KERNEL__)
    return true; // Always assume multicore testable in host/mock mode
#else
    extern uint32_t g_active_core_count;
    if (g_active_core_count >= 2) {
        return true;
    }
    return false;
#endif
}

bool test_sched_remote_enqueue_ipi(void) {
    if (!sched_test_multicore_available()) {
        KTEST_PRINT(" [SKIP] sched_remote_ipi requires >= 2 online cores\n");
        return true;
    }

    sched_init();

    // Create a new process and thread
    bh_process_t *proc = process_create("test_proc");
    KTEST_ASSERT(proc != NULL, "Failed to create process");

    bh_thread_t *thread = thread_create(proc, dummy_thread_entry);
    KTEST_ASSERT(thread != NULL, "Failed to create thread");

    // We mock that we are on CPU 0 and want to enqueue on CPU 1
    current_mock_cpu = 0;
    mock_ipi_sent_to = UINT32_MAX;

    // Enqueue thread to core 1
    int res = sched_enqueue(thread, 1);
    KTEST_ASSERT(res == 0, "Failed to enqueue thread on remote core");

    // Assert IPI was sent to core 1
    KTEST_ASSERT(mock_ipi_sent_to == 1, "IPI was not sent to target core");

    // Assert metrics
    sched_rq_t *rq_core1 = &g_cpu_locals[1].runqueue;
    KTEST_ASSERT(rq_core1->remote_enqueues == 1, "remote_enqueues counter mismatch");
    KTEST_ASSERT(rq_core1->ipi_sent == 1, "ipi_sent counter mismatch");
    KTEST_ASSERT(rq_core1->resched_pending == 1, "resched_pending flag not set");

    // Now switch to CPU 1 and simulate IPI reception by calling sched_reschedule
    current_mock_cpu = 1;
    sched_reschedule();

    // Assert the inbox was drained
    KTEST_ASSERT(list_empty(&rq_core1->pending_inbox), "Pending inbox was not drained");
    KTEST_ASSERT(rq_core1->inbox_drains == 1, "inbox_drains counter mismatch");
    KTEST_ASSERT(rq_core1->resched_pending == 0, "resched_pending flag not cleared");

    // Assert thread is now on the runqueue and runnable
    KTEST_ASSERT(rq_core1->runnable_count > 0, "Runqueue runnable_count is 0 after drain");

    // Verify preemption: check if remote_preemptions was incremented
    // (Since our new thread has priority 1 and idle has 0, it should preempt)
    KTEST_ASSERT(rq_core1->remote_preemptions == 1, "remote_preemptions counter mismatch");

    return true;
}

bool test_sched_ipi_coalescing(void) {
    if (!sched_test_multicore_available()) {
        return true; // Skip (logged by first test)
    }
    sched_init();

    bh_process_t *proc = process_create("test_proc2");

    bh_thread_t *thread1 = thread_create(proc, dummy_thread_entry);
    bh_thread_t *thread2 = thread_create(proc, dummy_thread_entry);

    current_mock_cpu = 0;
    mock_ipi_sent_to = UINT32_MAX;

    // First enqueue -> sets flag, sends IPI
    sched_enqueue(thread1, 1);

    // Reset mock IPI tracker
    mock_ipi_sent_to = UINT32_MAX;

    // Second enqueue -> sees flag, skips IPI, increments coalesced
    sched_enqueue(thread2, 1);

    KTEST_ASSERT(mock_ipi_sent_to == UINT32_MAX, "IPI should have been coalesced but was sent");

    sched_rq_t *rq_core1 = &g_cpu_locals[1].runqueue;
    KTEST_ASSERT(rq_core1->ipi_coalesced == 1, "ipi_coalesced counter mismatch");

    current_mock_cpu = 1;
    sched_reschedule(); // Drains both

    KTEST_ASSERT(rq_core1->inbox_drains == 1, "inbox_drains counter mismatch (drained together)");
    KTEST_ASSERT(list_empty(&rq_core1->pending_inbox), "Inbox should be empty");

    return true;
}

static ktest_case_t sched_ipi_tests[] = {
    {"Remote Enqueue IPI", test_sched_remote_enqueue_ipi},
    {"IPI Coalescing", test_sched_ipi_coalescing},
};

void ktest_sched_ipi_run(void) { ktest_run_suite("Scheduler IPI Unit Tests", sched_ipi_tests, 2); }

static int boot_test_sched_ipi(void) {
  if (test_sched_remote_enqueue_ipi() && test_sched_ipi_coalescing()) {
    return 0; // success
  }
  return -1;
}

REGISTER_BOOT_SELFTEST("sched_remote_ipi", "scheduler", boot_test_sched_ipi, BOOT_TEST_STAGE_RUNTIME, BOOT_TEST_EXTENDED, 0, true)