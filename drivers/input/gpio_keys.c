#include "gpio_keys.h"
#include "../core/device_registry.h"
#include "../core/driver_registry.h"
#include <stddef.h>

static int gpio_keys_probe(device_desc_t* dev) {
    if (!dev || dev->dev_class != CLASS_INPUT) return -1;
    return 0;
}

static void gpio_keys_remove(device_desc_t* dev) {
}

static driver_desc_t gpio_keys_driver = {
    .name = "gpio-keys",
    .supported_class = CLASS_INPUT,
    .match_compatible_id = "generic,gpio-keys",
    .probe = gpio_keys_probe,
    .remove = gpio_keys_remove
};

int gpio_keys_init(void) {
    return driver_register(&gpio_keys_driver);
}

int gpio_keys_device_register(device_desc_t* dev) {
    if (!dev) return -1;
    dev->dev_class = CLASS_INPUT;
    return device_register(dev);
}