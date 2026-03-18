#include <assert.h>
#include <stdio.h>

#include "../subsys/include/subsys.h"

// Stubs for subsys_test_runner.c
#include "../subsys/include/bharat/subsys_test.h"
const subsys_test_t __subsys_tests_start[0] = {};
const subsys_test_t __subsys_tests_end[0] = {};

void hal_serial_write(char c) {
    (void)c;
}

void kernel_panic(const char* msg) {
    (void)msg;
    while(1);
}


static void test_subsys_destroy_null_returns_error(void) {
    assert(-1 == subsys_destroy(NULL));
    printf("[PASS] test_subsys_destroy_null_returns_error\n");
}

static void test_subsys_destroy_stops_running_instance(void) {
    subsys_instance_t instance = {0};
    instance.is_running = 1;

    assert(0 == subsys_destroy(&instance));
    assert(0 == instance.is_running);
    printf("[PASS] test_subsys_destroy_stops_running_instance\n");
}

static void test_subsys_destroy_on_stopped_instance_is_safe(void) {
    subsys_instance_t instance = {0};
    instance.is_running = 0;

    assert(0 == subsys_destroy(&instance));
    assert(0 == instance.is_running);
    printf("[PASS] test_subsys_destroy_on_stopped_instance_is_safe\n");
}

int main(void) {
    printf("Running Subsystem Manager tests...\n");

    test_subsys_destroy_null_returns_error();
    test_subsys_destroy_stops_running_instance();
    test_subsys_destroy_on_stopped_instance_is_safe();

    printf("Subsystem Manager tests passed successfully.\n");
    return 0;
}
