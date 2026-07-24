#include <assert.h>
#include "../../core/drivers/include/driver_core.h"
#include "../../core/drivers/core/device_registry.h"
#include "../../core/drivers/core/driver_registry.h"
#include "../../core/drivers/core/event.h"
#include "../../core/services/core/devmgr/devmgr_skeleton.h"

// Bring in initializers
int gpio_core_init(void);
int gpio_controller_register(device_desc_t* dev);

int main() {
    // Initialize core subsystems
    assert(device_registry_init() == 0);
    assert(driver_registry_init() == 0);
    assert(devmgr_init() == 0);

    // Initialize driver modules
    assert(gpio_core_init() == 0);

    // Initial state check
    assert(devmgr_get_tracked_device_count() == 0);

    // Register a mock GPIO device
    device_desc_t mock_gpio_dev = {
        .name = "mock-gpio",
        .compatible_id = "generic,gpio"
    };

    int ret = gpio_controller_register(&mock_gpio_dev);
    assert(ret == 0);

    // The device should be tracked by devmgr after emitting EVENT_DEVICE_ADDED
    assert(devmgr_get_tracked_device_count() == 1);

    // Unregister the device
    device_unregister(&mock_gpio_dev);
    assert(devmgr_get_tracked_device_count() == 0);

    return 0;
}