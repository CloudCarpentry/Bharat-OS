#ifndef BHARAT_SCHED_TEST_SUPPORT_H
#define BHARAT_SCHED_TEST_SUPPORT_H

#include <stdint.h>

#if defined(BHARAT_ENABLE_KERNEL_SELFTESTS)

/*
 * Resets scheduler globals, frees runtime test threads,
 * and clears runqueues. Must be called before each
 * self-contained test to guarantee clean state.
 */
void sched_test_reset(void);

/*
 * Overrides the active core count for scheduler testing
 * without requiring real SMP hardware topology.
 */
void sched_set_test_core_count(uint32_t core_count);

#endif /* BHARAT_ENABLE_KERNEL_SELFTESTS */

#endif // BHARAT_SCHED_TEST_SUPPORT_H
