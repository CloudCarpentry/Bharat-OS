#include "bharat/subsys_test.h"
#include "boot/boot_args.h"
#include "hal/hal.h"
#include <stddef.h>

extern const subsys_test_t __subsys_tests_start[];
extern const subsys_test_t __subsys_tests_end[];

extern void kernel_panic(const char *message);

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

void subsys_run_boot_tests(const char *subsys_name) {
  if (!subsys_name) return;

  char test_arg[64] = {0};
  char fail_arg[64] = {0};
  bool is_all = false;
  bool is_quick = false;
  bool is_target_subsys = false;
  bool is_name_test = false;
  const char *target_name = NULL;

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
      const char *t_subsys = test_arg + 7;
      if (my_strcmp(t_subsys, subsys_name) == 0) {
        is_target_subsys = true;
      } else {
        return; // running for another subsystem
      }
    } else if (my_strncmp(test_arg, "name:", 5) == 0) {
      is_name_test = true;
      target_name = test_arg + 5;
    }
  }

  // Check bare flags if not fully parsed as key-value
  if (!is_all && !is_quick && !is_target_subsys && !is_name_test) {
      if (boot_has_flag("test=all")) is_all = true;
      else if (boot_has_flag("test=quick")) is_quick = true;
  }

  bool fail_panic = false;
  if (boot_get_kv("test_fail", fail_arg, sizeof(fail_arg))) {
    if (my_strcmp(fail_arg, "panic") == 0) {
      fail_panic = true;
    }
  }

  const subsys_test_t *test = __subsys_tests_start;
  uint32_t count = 0;

  // Let's do a quick pass to see if we have tests for this subsystem
  const subsys_test_t *t_iter = __subsys_tests_start;
  bool has_tests = false;
  while (t_iter < __subsys_tests_end) {
    if (t_iter->subsystem && my_strcmp(t_iter->subsystem, subsys_name) == 0) {
      has_tests = true;
      break;
    }
    t_iter++;
  }

  if (!has_tests) {
    return; // No tests for this subsystem
  }

  hal_serial_write("--- Running Tests for Subsystem: ");
  hal_serial_write(subsys_name);
  hal_serial_write(" ---\n");

  while (test < __subsys_tests_end) {
    bool should_run = false;

    // Only run if it's the target subsystem
    if (test->subsystem && my_strcmp(test->subsystem, subsys_name) == 0) {
      if (is_all) {
        should_run = true;
      } else if (is_target_subsys) {
        should_run = true;
      } else if (is_quick && test->quick) {
        should_run = true;
      } else if (is_name_test && test->name) {
        if (my_strcmp(test->name, target_name) == 0) {
          should_run = true;
        }
      }
    }

    if (should_run) {
      hal_serial_write(" [SUBSYS_TEST] ");
      hal_serial_write(test->name);
      hal_serial_write("... ");

      int result = test->run();

      if (result == 0) {
        hal_serial_write("PASSED\n");
      } else {
        hal_serial_write("FAILED\n");
        if (test->critical) {
          hal_serial_write("Critical subsystem test failed. ");
          kernel_panic("Critical subsystem test failed");
        } else if (fail_panic) {
          kernel_panic("Subsystem test failed");
        }
      }
      count++;
    }

    test++;
  }

  if (count > 0) {
    hal_serial_write("--- Subsystem Tests Complete ---\n");
  }
}
