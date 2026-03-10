#ifndef BHARAT_SUBSYS_TEST_H
#define BHARAT_SUBSYS_TEST_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Subsystem test definition structure.
 * Tests are placed in the `.subsys_tests` section and executed by the
 * subsystem manager when testing is requested for a specific subsystem.
 */
typedef struct {
  const char *subsystem; // E.g., "linux", "android", "vfs"
  const char *name;      // E.g., "syscall_translation_sanity"
  int (*run)(void);      // Test function, returns 0 on success
  uint8_t quick;         // 1 if fast test, 0 if long-running
  uint8_t critical;      // 1 if failure should cause panic/abort
} subsys_test_t;

/**
 * @brief Macro to register a subsystem test.
 *
 * @param t_subsys Name of the subsystem (e.g., "linux").
 * @param t_name Name of the test.
 * @param func Test function pointer.
 * @param quick_flag 1 if it's a quick test.
 * @param crit_flag 1 if failure is critical.
 */
#define REGISTER_SUBSYS_TEST(t_subsys, t_name, func, quick_flag, crit_flag)    \
  static const subsys_test_t __attribute__((used, section(".subsys_tests")))   \
      _subsys_test_##func = {t_subsys, t_name, func, quick_flag, crit_flag};

/**
 * @brief Run subsystem tests for a given subsystem.
 *
 * @param subsys_name The name of the subsystem to run tests for.
 */
void subsys_run_boot_tests(const char *subsys_name);

#endif // BHARAT_SUBSYS_TEST_H
