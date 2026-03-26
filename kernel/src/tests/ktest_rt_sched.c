#include "sched/sched.h"
#include "sched/sched_test_support.h"
#include "tests/ktest.h"
#include <bharat/cpu_local.h>

#if defined(TESTING) || defined(BHARAT_KERNEL_TESTS)
extern void sched_test_reset(void);
extern void sched_set_test_core_count(uint32_t core_count);
#endif
extern kthread_t *sched_pick_next_ready_l0(uint32_t core_id);

static void dummy_thread_entry(void) {
    while (1) {
        kthread_yield();
    }
}

static bool test_rms_admission(void) {
#if defined(TESTING) || defined(BHARAT_KERNEL_TESTS)
    sched_set_test_core_count(1);
    sched_test_reset();
#endif
    sched_init();

    kprocess_t *p = process_create("rms_test");
    KTEST_ASSERT(p != NULL, "Process creation failed");

    // Total RMS allowed budget is ~69% (690 in our unit)

    // Create Task 1: WCET 20ms, Period 100ms (20%)
    kthread_t *t1 = thread_create_detached(p, dummy_thread_entry);
    KTEST_ASSERT(t1 != NULL, "Thread 1 creation failed");
    int ret1 = sched_admission_rms(t1, 20, 100);
    KTEST_ASSERT(ret1 == 0, "Admission of T1 should succeed");

    // Create Task 2: WCET 40ms, Period 100ms (40%)
    kthread_t *t2 = thread_create_detached(p, dummy_thread_entry);
    KTEST_ASSERT(t2 != NULL, "Thread 2 creation failed");
    int ret2 = sched_admission_rms(t2, 40, 100);
    KTEST_ASSERT(ret2 == 0, "Admission of T2 should succeed");

    // Total used = 600 (60%)

    // Create Task 3: WCET 15ms, Period 100ms (15%)
    // This will bring total used to 750 (75%), which is > 690, so it should be rejected.
    kthread_t *t3 = thread_create_detached(p, dummy_thread_entry);
    KTEST_ASSERT(t3 != NULL, "Thread 3 creation failed");
    int ret3 = sched_admission_rms(t3, 15, 100);
    KTEST_ASSERT(ret3 != 0, "Admission of T3 should fail due to bandwidth bounds");

    // Clean up
    thread_destroy(t1);
    thread_destroy(t2);
    thread_destroy(t3);
    process_destroy(p);
    return true;
}

static bool test_edf_admission_and_queue(void) {
#if defined(TESTING) || defined(BHARAT_KERNEL_TESTS)
    sched_set_test_core_count(1);
    sched_test_reset();
#endif
    sched_init();
    sched_set_policy(SCHED_POLICY_EDF);

    kprocess_t *p = process_create("edf_test");
    KTEST_ASSERT(p != NULL, "Process creation failed");

    // Total EDF allowed budget is 100% (1000 in our unit)

    // Create Task 1: WCET 30ms, Period 100ms, Deadline 100ms (30%)
    kthread_t *t1 = thread_create_detached(p, dummy_thread_entry);
    int ret1 = sched_admission_edf(t1, 30, 100, 100);
    KTEST_ASSERT(ret1 == 0, "Admission of T1 should succeed");

    // Create Task 2: WCET 50ms, Period 100ms, Deadline 50ms (50%)
    kthread_t *t2 = thread_create_detached(p, dummy_thread_entry);
    int ret2 = sched_admission_edf(t2, 50, 100, 50);
    KTEST_ASSERT(ret2 == 0, "Admission of T2 should succeed");

    // Create Task 3: WCET 30ms, Period 100ms, Deadline 100ms (30%)
    // Total used = 300 + 500 = 800. Adding 300 -> 1100 > 1000. Should reject.
    kthread_t *t3 = thread_create_detached(p, dummy_thread_entry);
    int ret3 = sched_admission_edf(t3, 30, 100, 100);
    KTEST_ASSERT(ret3 != 0, "Admission of T3 should fail (budget exceeded)");

    // Enqueue T1 and T2
    sched_enqueue(t1, 0);
    sched_enqueue(t2, 0);

    // T1 absolute deadline = ticks + 100 = 100 (assuming ticks=0)
    // T2 absolute deadline = ticks + 50 = 50

    // Pick next ready should return T2 because its deadline (50) is smaller than T1 (100)
    kthread_t *picked1 = sched_pick_next_ready_l0(0);
    KTEST_ASSERT(picked1 == t2, "EDF should pick T2 because it has earliest deadline");

    // Second pick should return T1
    kthread_t *picked2 = sched_pick_next_ready_l0(0);
    KTEST_ASSERT(picked2 == t1, "EDF should pick T1 next");

    // Clean up
    thread_destroy(t1);
    thread_destroy(t2);
    thread_destroy(t3);
    process_destroy(p);

    sched_set_policy(SCHED_POLICY_CLOUD_FAIR); // restore default
    return true;
}

int boot_test_rt_sched(void) {
    ktest_case_t rt_tests[] = {
        {"RMS Admission Control", test_rms_admission},
        {"EDF Admission & Sorting", test_edf_admission_and_queue}
    };

    ktest_run_suite("Real-Time Scheduler Tests", rt_tests, 2);
    return 0; // Success if all tests inside suite passed
}

REGISTER_BOOT_SELFTEST("rt_sched", "scheduler", boot_test_rt_sched, BOOT_TEST_STAGE_RUNTIME, BOOT_TEST_MANDATORY, 0, false)
