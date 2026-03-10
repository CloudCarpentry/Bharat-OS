#include "boot/boot_args.h"
#include "hal/hal.h"
#include "kernel.h"
#include "tests/ktest.h"

extern const kernel_test_t __kern_tests_start[];
extern const kernel_test_t __kern_tests_end[];

static int my_strcmp(const char *s1, const char *s2) {
  while (*s1 && (*s1 == *s2)) {
    s1++;
    s2++;
  }
  return *(const unsigned char *)s1 - *(const unsigned char *)s2;
}

static int my_strncmp(const char *s1, const char *s2, size_t n) {
  for (size_t i = 0; i < n; i++) {
    if (s1[i] != s2[i]) {
      return (unsigned char)s1[i] - (unsigned char)s2[i];
    }
    if (s1[i] == '\0') {
      return 0;
    }
  }
  return 0;
}

extern void kernel_panic(const char *message);

void kernel_run_boot_tests(void) {
  char test_arg[64] = {0};
  char fail_arg[64] = {0};
  bool is_all = false;
  bool is_quick = false;
  bool is_subsys_test = false;
  bool is_name_test = false;
  const char *target_subsys = NULL;
  const char *target_name = NULL;

  // Default to off in normal builds, maybe 'quick' in debug.
  // We'll read the cmdline
  if (!boot_get_kv("test", test_arg, sizeof(test_arg))) {
#ifdef CONFIG_BOOT_TEST_DEFAULT_QUICK
    is_quick = true;
#else
    return; // test=off by default
#endif
  } else {
    if (my_strcmp(test_arg, "off") == 0) {
      return;
    } else if (my_strcmp(test_arg, "quick") == 0) {
      is_quick = true;
    } else if (my_strcmp(test_arg, "all") == 0) {
      is_all = true;
    } else if (my_strncmp(test_arg, "subsys:", 7) == 0) {
      is_subsys_test = true;
      target_subsys = test_arg + 7;
    } else if (my_strncmp(test_arg, "name:", 5) == 0) {
      is_name_test = true;
      target_name = test_arg + 5;
    }
  }

  // Check bare flags if not fully parsed as key-value
  if (!is_all && !is_quick && !is_subsys_test && !is_name_test) {
      if (boot_has_flag("test=all")) is_all = true;
      else if (boot_has_flag("test=quick")) is_quick = true;
  }

  bool fail_panic = false;
  if (boot_get_kv("test_fail", fail_arg, sizeof(fail_arg))) {
    if (my_strcmp(fail_arg, "panic") == 0) {
      fail_panic = true;
    }
  }

  hal_serial_write("--- Running Boot Kernel Tests ---\n");

  const kernel_test_t *test = __kern_tests_start;
  uint32_t passed = 0;
  uint32_t failed = 0;
  uint32_t skipped = 0;

  while (test < __kern_tests_end) {
    bool should_run = false;

    if (is_all) {
      should_run = true;
    } else if (is_quick && test->quick) {
      should_run = true;
    } else if (is_subsys_test && test->group) {
      if (my_strcmp(test->group, target_subsys) == 0) {
        should_run = true;
      }
    } else if (is_name_test && test->name) {
      if (my_strcmp(test->name, target_name) == 0) {
        should_run = true;
      }
    }

    if (should_run) {
      hal_serial_write(" [TEST] ");
      hal_serial_write(test->name);
      hal_serial_write("... ");

      int result = test->run();

      if (result == 0) {
        hal_serial_write("PASSED\n");
        passed++;
      } else {
        hal_serial_write("FAILED\n");
        failed++;

        if (test->critical) {
          hal_serial_write("Critical test failed. ");
          // Always panic on critical failure in boot tests
          kernel_panic("Critical boot test failed");
        } else if (fail_panic) {
          kernel_panic("Boot test failed");
        }
      }
    } else {
      skipped++;
    }

    test++;
  }

  hal_serial_write("--- Boot Tests Complete ---\n");
}
