#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "device_registry.h"
#include "driver_registry.h"
#include "bharat/uapi/sys_errno.h"

// Mock probe function
static int mock_probe(device_desc_t* dev) {
    return 0;
}

void test_device_registration_hardening() {
    printf("Running test_device_registration_hardening...\n");
    device_registry_init();

    device_desc_t dev1 = { .name = "test-dev-1" };
    device_desc_t dev_no_name = { .name = "" };
    device_desc_t dev_null_name = { .name = NULL };

    // Valid registration
    assert(device_register(&dev1) == 0);
    assert(device_registry_get_count() == 1);

    // Reject NULL
    assert(device_register(NULL) == -SYS_EINVAL);
    assert(device_registry_get_count() == 1);

    // Reject empty/NULL name
    assert(device_register(&dev_no_name) == -SYS_EINVAL);
    assert(device_register(&dev_null_name) == -SYS_EINVAL);
    assert(device_registry_get_count() == 1);

    // Reject duplicate
    device_desc_t dev1_dup = { .name = "test-dev-1" };
    assert(device_register(&dev1_dup) == -SYS_EEXIST);
    assert(device_registry_get_count() == 1);

    printf("test_device_registration_hardening passed!\n");
}

void test_driver_registration_hardening() {
    printf("Running test_driver_registration_hardening...\n");
    driver_registry_init();

    driver_desc_t drv1 = { .name = "test-drv-1" };

    // Valid registration
    assert(driver_register(&drv1) == 0);
    assert(driver_registry_get_count() == 1);

    // Reject NULL
    assert(driver_register(NULL) == -SYS_EINVAL);

    // Reject duplicate
    driver_desc_t drv1_dup = { .name = "test-drv-1" };
    assert(driver_register(&drv1_dup) == -SYS_EEXIST);
    assert(driver_registry_get_count() == 1);

    printf("test_driver_registration_hardening passed!\n");
}

int main() {
    test_device_registration_hardening();
    test_driver_registration_hardening();
    printf("All registry hardening tests passed!\n");
    return 0;
}
