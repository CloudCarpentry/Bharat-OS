#include "bharat_config.h"
#include "mm.h"
#include "numa.h"
#include "tests/ktest.h"
#include <stddef.h>

bool test_pmm_single_page_alloc_free(void) {
  phys_addr_t p1 = mm_alloc_page(NUMA_NODE_ANY);
  KTEST_ASSERT(p1 != 0, "p1 allocation failed");

  phys_addr_t p2 = mm_alloc_page(NUMA_NODE_ANY);
  KTEST_ASSERT(p2 != 0, "p2 allocation failed");
  KTEST_ASSERT(p1 != p2, "p1 and p2 must be different");

  mm_free_page(p1);
  mm_free_page(p2);

  return true;
}

bool test_pmm_order_alloc(void) {
  // Allocate 4 contiguous pages (order 2)
  phys_addr_t p = mm_alloc_pages_order(2, NUMA_NODE_ANY, PAGE_FLAG_KERNEL);
  KTEST_ASSERT(p != 0, "order 2 allocation failed");
  KTEST_ASSERT((p % (PAGE_SIZE * 4)) == 0,
               "order 2 address must be 16KB aligned");

  mm_free_page(p); // Buddy system should reclaim based on order correctly
  return true;
}


#include "mm/pmm_pcache.h"

bool test_pmm_remote_free_bounds(void) {
    if (MAX_SUPPORTED_CORES < 2) return true; // Need SMP

    pmm_core_state_t *target = &g_pmm_cores[1];
    spin_lock(&target->inbox.lock);

    // Simulate enqueuing PMM_INBOX_SIZE + 1 items
    uint32_t queued = 0;
    uint32_t failed = 0;

    for (int i = 0; i < PMM_INBOX_SIZE + 1; i++) {
        uint32_t next_head = (target->inbox.head + 1) % PMM_INBOX_SIZE;
        if (next_head != target->inbox.tail) {
            target->inbox.pages[target->inbox.head] = 0x1000 * i;
            target->inbox.head = next_head;
            target->inbox.enqueue_count++;
            queued++;
        } else {
            target->inbox.enqueue_failures++;
            failed++;
        }
    }
    spin_unlock(&target->inbox.lock);

    KTEST_ASSERT(queued == PMM_INBOX_SIZE - 1, "Inbox failed to bound enqueues");
    KTEST_ASSERT(failed == 2, "Inbox failed to reject overflow enqueues");

    // Clean up
    spin_lock(&target->inbox.lock);
    target->inbox.head = 0;
    target->inbox.tail = 0;
    spin_unlock(&target->inbox.lock);

    return true;
}


bool test_pmm_buddy_merging(void) {
  // This is a bit tricky to test without knowing initial state,
  // but we can try to allocate two adjacent pages and see if we can get a
  // larger one later.
  phys_addr_t p1 = mm_alloc_page(NUMA_NODE_ANY);
  phys_addr_t p2 = mm_alloc_page(NUMA_NODE_ANY);

  // We don't guarantee p1 and p2 are buddies, but if we free enough and then
  // ask for order 1, it should work if the system is stable.
  mm_free_page(p1);
  mm_free_page(p2);

  phys_addr_t p3 = mm_alloc_pages_order(1, NUMA_NODE_ANY, PAGE_FLAG_KERNEL);
  KTEST_ASSERT(p3 != 0, "order 1 allocation failed after freeing");
  mm_free_page(p3);

  return true;
}

static ktest_case_t pmm_tests[] = {
    {"PMM Single Page", test_pmm_single_page_alloc_free},
    {"PMM Order Alloc", test_pmm_order_alloc},
    {"PMM Buddy Merging", test_pmm_buddy_merging},
    {"PMM Remote Free Bounds", test_pmm_remote_free_bounds},
};

void ktest_pmm_run(void) { ktest_run_suite("PMM Unit Tests", pmm_tests, 4); }

static int boot_test_pmm_alloc(void) {
  if (test_pmm_single_page_alloc_free()) {
    return 0; // success
  }
  return -1;
}

REGISTER_BOOT_SELFTEST("pmm_alloc_free", "allocator", boot_test_pmm_alloc, BOOT_TEST_STAGE_MEMORY, BOOT_TEST_MANDATORY, 0, true)
