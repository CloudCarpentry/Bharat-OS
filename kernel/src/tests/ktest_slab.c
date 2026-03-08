#include "slab.h"
#include "tests/ktest.h"
#include <stddef.h>


bool test_kmalloc_basic(void) {
  void *p1 = kmalloc(32);
  KTEST_ASSERT(p1 != NULL, "32 byte kmalloc failed");

  void *p2 = kmalloc(64);
  KTEST_ASSERT(p2 != NULL, "64 byte kmalloc failed");
  KTEST_ASSERT(p1 != p2, "p1 and p2 must be different");

  kfree(p1);
  kfree(p2);
  return true;
}

bool test_kmalloc_large(void) {
  // Allocation larger than 2048 should fall back to page allocation
  void *p = kmalloc(4096);
  KTEST_ASSERT(p != NULL, "4096 byte kmalloc failed");
  KTEST_ASSERT(((uintptr_t)p % 4096) == 0,
               "4096 byte allocation must be page aligned");

  kfree(p);
  return true;
}

bool test_kmalloc_multiple_same_size(void) {
  // Fill a page with 128 byte objects (32 per page)
  void *ptrs[33];
  for (int i = 0; i < 33; i++) {
    ptrs[i] = kmalloc(128);
    KTEST_ASSERT(ptrs[i] != NULL, "kmalloc(128) failed in loop");
  }

  // All must be unique
  for (int i = 0; i < 33; i++) {
    for (int j = i + 1; j < 33; j++) {
      KTEST_ASSERT(ptrs[i] != ptrs[j], "Duplicate pointers detected");
    }
  }

  for (int i = 0; i < 33; i++) {
    kfree(ptrs[i]);
  }
  return true;
}

static ktest_case_t slab_tests[] = {
    {"kmalloc Basic", test_kmalloc_basic},
    {"kmalloc Large", test_kmalloc_large},
    {"kmalloc Multiple", test_kmalloc_multiple_same_size},
};

void ktest_slab_run(void) { ktest_run_suite("Slab Unit Tests", slab_tests, 3); }
