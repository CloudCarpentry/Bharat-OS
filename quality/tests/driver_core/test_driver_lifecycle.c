#include <stdio.h>
#include <assert.h>
#include "driver_core.h"
#include "binding.h"
#include "device_registry.h"
#include "driver_registry.h"
#include "bharat/uapi/sys_errno.h"

static int probe_call_count = 0;
static int mock_probe(device_desc_t* dev) {
    probe_call_count++;
    return 0;
}

static int mock_probe_fail(device_desc_t* dev) {
    return -SYS_EIO;
}

void test_lifecycle_basic() {
    printf("Running test_lifecycle_basic...\n");
    device_registry_init();
    driver_registry_init();

    driver_desc_t drv = {
        .name = "test-drv",
        .match_class = CLASS_GPIO,
        .probe = mock_probe
    };
    driver_register(&drv);

    device_desc_t dev = {
        .name = "test-dev",
        .dev_class = CLASS_GPIO
    };
    device_register(&dev);

    device_binding_t* binding = device_binding_find_by_dev(&dev);
    assert(binding != NULL);
    assert(binding->state == DRIVER_STATE_MATCHED);

    // MATCHED -> PROBED
    probe_call_count = 0;
    assert(device_binding_probe(binding) == 0);
    assert(binding->state == DRIVER_STATE_PROBED);
    assert(probe_call_count == 1);

    // PROBED -> STARTED
    assert(device_binding_start(binding) == 0);
    assert(binding->state == DRIVER_STATE_STARTED);

    // STARTED -> STOPPED
    assert(device_binding_stop(binding) == 0);
    assert(binding->state == DRIVER_STATE_STOPPED);

    // STOPPED -> REMOVED
    assert(device_binding_remove(binding) == 0);
    // binding should now be invalidated/cleared
    assert(device_binding_find_by_dev(&dev) == NULL);

    printf("test_lifecycle_basic passed!\n");
}

void test_lifecycle_invalid_transitions() {
    printf("Running test_lifecycle_invalid_transitions...\n");
    device_registry_init();
    driver_registry_init();

    driver_desc_t drv = {
        .name = "test-drv",
        .match_class = CLASS_GPIO,
        .probe = mock_probe
    };
    driver_register(&drv);

    device_desc_t dev = {
        .name = "test-dev",
        .dev_class = CLASS_GPIO
    };
    device_register(&dev);

    device_binding_t* binding = device_binding_find_by_dev(&dev);
    assert(binding != NULL);

    // Start without probe should fail
    assert(device_binding_start(binding) == -SYS_EPERM);
    assert(binding->state == DRIVER_STATE_MATCHED);

    // Probe then start
    assert(device_binding_probe(binding) == 0);
    assert(device_binding_start(binding) == 0);

    // Probe while started should fail
    assert(device_binding_probe(binding) == -SYS_EPERM);

    printf("test_lifecycle_invalid_transitions passed!\n");
}

void test_lifecycle_failure() {
    printf("Running test_lifecycle_failure...\n");
    device_registry_init();
    driver_registry_init();

    driver_desc_t drv_fail = {
        .name = "fail-drv",
        .match_class = CLASS_GPIO,
        .probe = mock_probe_fail
    };
    driver_register(&drv_fail);

    device_desc_t dev = {
        .name = "test-dev",
        .dev_class = CLASS_GPIO
    };
    device_register(&dev);

    device_binding_t* binding = device_binding_find_by_dev(&dev);
    assert(binding != NULL);

    // Probe fails
    assert(device_binding_probe(binding) == -SYS_EIO);
    assert(binding->state == DRIVER_STATE_FAILED);

    // Start from FAILED should fail
    assert(device_binding_start(binding) == -SYS_EPERM);

    // Retry probe from FAILED
    drv_fail.probe = mock_probe; // "Fix" the driver
    assert(device_binding_probe(binding) == 0);
    assert(binding->state == DRIVER_STATE_PROBED);

    printf("test_lifecycle_failure passed!\n");
}

void test_lifecycle_binding_reuse() {
    printf("Running test_lifecycle_binding_reuse...\n");
    device_registry_init();
    driver_registry_init();

    driver_desc_t drv = { .name = "drv", .match_class = CLASS_GPIO };
    driver_register(&drv);

    device_desc_t dev = { .name = "dev", .dev_class = CLASS_GPIO };

    // Perform many register/unregister cycles to ensure we don't leak bindings
    for (int i = 0; i < 200; i++) {
        assert(device_register(&dev) == 0);
        device_binding_t* b = device_binding_find_by_dev(&dev);
        assert(b != NULL);
        assert(device_binding_remove(b) == 0);
        device_unregister(&dev);
    }

    printf("test_lifecycle_binding_reuse passed!\n");
}

int main() {
    test_lifecycle_basic();
    test_lifecycle_invalid_transitions();
    test_lifecycle_failure();
    test_lifecycle_binding_reuse();
    printf("All lifecycle tests passed!\n");
    return 0;
}
