#include "sched/sched_test_support.h"

#if defined(BHARAT_ENABLE_KERNEL_SELFTESTS)

#include "sched_internal.h"
#include "arch/arch_ext_state.h"
#include "capability.h"
#include "lib/base/string.h"
#include "slab.h"
#include "mm/mm_aspace_switch.h"

void sched_set_test_core_count(uint32_t core_count) {
  if (core_count == 0U) {
    g_sched_test_core_count = 1U;
  } else if (core_count > MAX_SUPPORTED_CORES) {
    g_sched_test_core_count = MAX_SUPPORTED_CORES;
  } else {
    g_sched_test_core_count = core_count;
  }
}

void sched_test_reset(void) {
  // Use a simple spin lock or just disable interrupts for test reset
  // Since this is a test environment reset, we can afford to be heavy-handed.
  for (size_t i = 0; i < BHARAT_ARRAY_SIZE(g_threads); ++i) {
    if (g_threads[i].in_use != 0U) {
      g_threads[i].in_use = 0U; // Mark immediately to avoid reaper racing
      if (g_threads[i].thread.capability_list) {
        cap_table_destroy(g_threads[i].thread.capability_list);
        g_threads[i].thread.capability_list = NULL;
      }
      arch_ext_state_thread_destroy(&g_threads[i].thread);
      if (g_threads[i].thread.kernel_stack != 0U) {
        void *stack = (void *)(uintptr_t)g_threads[i].thread.kernel_stack;
        g_threads[i].thread.kernel_stack = 0U; // Set to 0 BEFORE kfree
        kfree(stack);
      }
    }
  }
  for (size_t i = 0; i < BHARAT_ARRAY_SIZE(g_processes); ++i) {
    if (g_processes[i].in_use != 0U) {
      g_processes[i].in_use = 0U;
      if (g_processes[i].process.security_sandbox_ctx) {
        cap_table_destroy(g_processes[i].process.security_sandbox_ctx);
        g_processes[i].process.security_sandbox_ctx = NULL;
      }
      if (g_processes[i].process.addr_space) {
        (void)aspace_destroy(g_processes[i].process.addr_space);
        g_processes[i].process.addr_space = NULL;
      }
    }
  }
  memset(g_bootstrap_threads, 0, sizeof(g_bootstrap_threads));
  sched_reset_core_runqueues();
  g_sched_initialized = 0U;
}

#endif /* BHARAT_ENABLE_KERNEL_SELFTESTS */
