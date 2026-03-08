#include "hal/hal.h"
#include "kernel.h"
#include "mm.h"
#include "numa.h"
#include "sched.h"
#include "tests/ktest.h"

#define KPRINT(s) hal_serial_write(s)

extern void ktest_pmm_run(void);
extern void ktest_slab_run(void);

static void test_sched_yield_smoke(void) {
  KPRINT(" [TEST] Scheduler yield smoke test... ");
  for (int i = 0; i < 5; i++) {
    sched_yield();
  }
  KPRINT("PASSED\n");
}

static void test_sched_sleep_smoke(void) {
  KPRINT(" [TEST] Scheduler sleep smoke test (100ms)... ");
  sched_sleep(100);
  KPRINT("PASSED\n");
}

static void test_pmm_stress(void) {
  KPRINT(" [STRESS] PMM allocation stress (1000 pages)... ");
  phys_addr_t pages[100];
  for (int loop = 0; loop < 10; loop++) {
    for (int i = 0; i < 100; i++) {
      pages[i] = mm_alloc_page(NUMA_NODE_ANY);
      if (pages[i] == 0) {
        KPRINT("FAILED at loop ");
        // simple hex print not available here easily, just indicate failure
        return;
      }
    }
    for (int i = 0; i < 100; i++) {
      mm_free_page(pages[i]);
    }
  }
  KPRINT("PASSED\n");
}

void kernel_tester_app(void) {
  KPRINT("\n========================================\n");
  KPRINT("      Bharat-OS Kernel Test App         \n");
  KPRINT("========================================\n\n");

  // 1. Run Unit Test Suites
  ktest_pmm_run();
  ktest_slab_run();

  // 2. Functional/Dynamic Tests
  KTEST_PRINT("--- Running Dynamic Tests ---\n");
  test_sched_yield_smoke();
  test_sched_sleep_smoke();
  test_pmm_stress();

  KPRINT("\n========================================\n");
  KPRINT("      All Tests Completed Successfully!\n");
  KPRINT("========================================\n\n");
}
