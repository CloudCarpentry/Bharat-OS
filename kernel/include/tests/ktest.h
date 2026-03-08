#ifndef BHARAT_KTEST_H
#define BHARAT_KTEST_H

#include "hal/hal.h"
#include <stdbool.h>
#include <stdint.h>


#define KTEST_PRINT(s) hal_serial_write(s)

typedef struct {
  const char *name;
  bool (*test_fn)(void);
} ktest_case_t;

#define KTEST_ASSERT(cond, msg)                                                \
  if (!(cond)) {                                                               \
    KTEST_PRINT("  [FAIL] ");                                                  \
    KTEST_PRINT(msg);                                                          \
    KTEST_PRINT("\n");                                                         \
    return false;                                                              \
  }

static inline void ktest_run_suite(const char *suite_name, ktest_case_t *tests,
                                   uint32_t count) {
  uint32_t passed = 0;
  KTEST_PRINT("--- Running Suite: ");
  KTEST_PRINT(suite_name);
  KTEST_PRINT(" ---\n");

  for (uint32_t i = 0; i < count; i++) {
    KTEST_PRINT(" [TEST] ");
    KTEST_PRINT(tests[i].name);
    KTEST_PRINT("... ");

    if (tests[i].test_fn()) {
      KTEST_PRINT("PASSED\n");
      passed++;
    }
  }

  KTEST_PRINT("--- Summary: ");
  if (passed == count) {
    KTEST_PRINT("ALL PASSED (");
  } else {
    KTEST_PRINT("SOME FAILED (");
  }
  // Simple hex print for numbers if needed, but for small counts decimal is
  // fine if we had it. For now just indicate completion.
  KTEST_PRINT("Done) ---\n\n");
}

#endif // BHARAT_KTEST_H
