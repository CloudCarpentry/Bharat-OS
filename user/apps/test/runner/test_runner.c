#include "../test_framework.h"

// Define maximum number of tests for the runner
#define MAX_TESTS 32

static test_case_t g_tests[MAX_TESTS];
static int g_test_count = 0;

void register_test(const char* name, int (*run)(void)) {
    if (g_test_count < MAX_TESTS) {
        g_tests[g_test_count].name = name;
        g_tests[g_test_count].run = run;
        g_test_count++;
    }
}

static int my_strcmp(const char *s1, const char *s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(const unsigned char *)s1 - *(const unsigned char *)s2;
}

void run_test(const char* name) {
    for (int i = 0; i < g_test_count; i++) {
        if (my_strcmp(g_tests[i].name, name) == 0) {
            console_print("[TEST_RUNNER] Running ");
            console_print(name);
            console_print("...\n");
            int result = g_tests[i].run();
            if (result == 0) {
                console_print("[TEST_RUNNER] ");
                console_print(name);
                console_print(" PASSED\n");
            } else {
                console_print("[TEST_RUNNER] ");
                console_print(name);
                console_print(" FAILED\n");
            }
            return;
        }
    }
    console_print("[TEST_RUNNER] Test not found: ");
    console_print(name);
    console_print("\n");
}

void run_all_tests(void) {
    console_print("[TEST_RUNNER] Running all tests...\n");
    for (int i = 0; i < g_test_count; i++) {
        run_test(g_tests[i].name);
    }
    console_print("[TEST_RUNNER] Finished running all tests.\n");
}
