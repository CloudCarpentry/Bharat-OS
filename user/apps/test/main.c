#include "test_framework.h"

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;

    console_print("[APP_TEST] Starting User-Space Test Framework\n");

    register_test("pmm_stress", pmm_stress_test_run);
    register_test("vmm_stress", vmm_stress_test_run);

    run_all_tests();

    console_print("[APP_TEST] Framework Complete.\n");
    return 0;
}
