#include "gpio_core.h"
#include "../../core/device_registry.h"
#include "../../core/driver_registry.h"
#include <stddef.h>

static int gpio_probe(device_desc_t* dev) {
    if (!dev || dev->dev_class != CLASS_GPIO) return -1;
    // Initialization logic for the GPIO controller
    return 0;
}

static void gpio_remove(device_desc_t* dev) {
    // Cleanup logic
}

static driver_desc_t gpio_driver = {
    .name = "gpio-generic",
    .supported_class = CLASS_GPIO,
    .match_compatible_id = "generic,gpio",
    .probe = gpio_probe,
    .remove = gpio_remove,
    .suspend = NULL,
    .resume = NULL,
    .reset = NULL,
    .fault = NULL
};

int gpio_core_init(void) {
    return driver_register(&gpio_driver);
}

int gpio_controller_register(device_desc_t* dev) {
    if (!dev) return -1;
    dev->dev_class = CLASS_GPIO;
    return device_register(dev);
}