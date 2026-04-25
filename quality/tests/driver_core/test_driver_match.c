#include <stdio.h>
#include <assert.h>
#include "match.h"
#include "device_registry.h"
#include "driver_registry.h"
#include "bharat/uapi/sys_errno.h"

static int probe_count = 0;
static int mock_probe(device_desc_t* dev) {
    probe_count++;
    return 0;
}

void test_match_scoring() {
    printf("Running test_match_scoring...\n");
    device_registry_init();
    driver_registry_init();

    // 1. Generic driver
    driver_desc_t drv_generic = {
        .name = "generic-drv",
        .supported_class = CLASS_GPIO,
        .probe = mock_probe,
        .priority = 0
    };

    // 2. Class-specific driver
    driver_desc_t drv_class = {
        .name = "class-drv",
        .match_class = CLASS_GPIO,
        .probe = mock_probe,
        .priority = 10
    };

    // 3. Compatible driver
    driver_desc_t drv_compat = {
        .name = "compat-drv",
        .match_compatible_id = "vendor,device-v1",
        .probe = mock_probe,
        .priority = 20
    };

    driver_register(&drv_generic);
    driver_register(&drv_class);
    driver_register(&drv_compat);

    device_desc_t dev = {
        .name = "test-device",
        .dev_class = CLASS_GPIO,
        .compatible_id = "vendor,device-v1"
    };

    probe_count = 0;
    assert(driver_match_device(&dev) == 0);
    // Should match drv_compat because it has the highest score
    assert(dev.driver_data == &drv_compat);
    assert(probe_count == 1);

    // Now test with a device that only matches class
    device_desc_t dev_class_only = {
        .name = "test-device-2",
        .dev_class = CLASS_GPIO,
        .compatible_id = "vendor,unknown-v1"
    };
    assert(driver_match_device(&dev_class_only) == 0);
    assert(dev_class_only.driver_data == &drv_class);

    printf("test_match_scoring passed!\n");
}

void test_match_tie_breaker() {
    printf("Running test_match_tie_breaker...\n");
    device_registry_init();
    driver_registry_init();

    driver_desc_t drv1 = {
        .name = "drv1",
        .match_class = CLASS_GPIO,
        .probe = mock_probe,
        .priority = 10
    };

    driver_desc_t drv2 = {
        .name = "drv2",
        .match_class = CLASS_GPIO,
        .probe = mock_probe,
        .priority = 20 // Higher priority
    };

    driver_register(&drv1);
    driver_register(&drv2);

    device_desc_t dev = {
        .name = "test-device",
        .dev_class = CLASS_GPIO
    };

    assert(driver_match_device(&dev) == 0);
    assert(dev.driver_data == &drv2); // Higher priority wins

    printf("test_match_tie_breaker passed!\n");
}

int main() {
    test_match_scoring();
    test_match_tie_breaker();
    printf("All match scoring tests passed!\n");
    return 0;
}
