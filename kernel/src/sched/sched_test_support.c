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

  // Clean up per-core thread pools
  for (uint32_t core = 0; core < MAX_SUPPORTED_CORES; ++core) {
    sched_rq_t *rq = &g_cpu_locals[core].runqueue;
    thread_slot_t *threads = (thread_slot_t*)rq->threads;
    process_slot_t *processes = (process_slot_t*)rq->processes;
    if (threads) {
      for (size_t i = 0; i < SCHED_MAX_THREADS; ++i) {
        if (threads[i].in_use != 0U) {
          threads[i].in_use = 0U; // Mark immediately to avoid reaper racing
          if (threads[i].thread.capability_list) {
            cap_table_destroy(threads[i].thread.capability_list);
            threads[i].thread.capability_list = NULL;
          }
          arch_ext_state_thread_destroy(&threads[i].thread);
          if (threads[i].thread.kernel_stack != 0U) {
            void *stack = (void *)(uintptr_t)threads[i].thread.kernel_stack;
            threads[i].thread.kernel_stack = 0U; // Set to 0 BEFORE kfree
            kfree(stack);
          }
        }
      }
    }
    if (processes) {
      for (size_t i = 0; i < SCHED_MAX_PROCESSES; ++i) {
        if (processes[i].in_use != 0U) {
          processes[i].in_use = 0U;
          if (processes[i].process.security_sandbox_ctx) {
            cap_table_destroy(processes[i].process.security_sandbox_ctx);
            processes[i].process.security_sandbox_ctx = NULL;
          }
          if (processes[i].process.addr_space) {
            aspace_destroy(processes[i].process.addr_space);
            processes[i].process.addr_space = NULL;
          }
        }
      }
    }
  }
  sched_reset_core_runqueues();
  g_sched_initialized = 0U;
}

#endif /* BHARAT_ENABLE_KERNEL_SELFTESTS */
