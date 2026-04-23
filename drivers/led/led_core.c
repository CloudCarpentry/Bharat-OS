#include "led_core.h"
#include "../core/device_registry.h"
#include "../core/driver_registry.h"
#include <stddef.h>

static int led_probe(device_desc_t* dev) {
    if (!dev || dev->dev_class != CLASS_LED) return -1;
    return 0;
}

static void led_remove(device_desc_t* dev) {
}

static driver_desc_t led_driver = {
    .name = "led-generic",
    .supported_class = CLASS_LED,
    .match_compatible_id = "generic,led",
    .probe = led_probe,
    .remove = led_remove
};

int led_core_init(void) {
    return driver_register(&led_driver);
}

int led_device_register(device_desc_t* dev) {
    if (!dev) return -1;
    dev->dev_class = CLASS_LED;
    return device_register(dev);
}