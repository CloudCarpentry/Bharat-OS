#include "pinctrl_core.h"
#include "../../core/device_registry.h"
#include "../../core/driver_registry.h"
#include <stddef.h>

static int pinctrl_probe(device_desc_t* dev) {
    if (!dev || dev->dev_class != CLASS_PINCTRL) return -1;
    return 0;
}

static void pinctrl_remove(device_desc_t* dev) {
}

static driver_desc_t pinctrl_driver = {
    .name = "pinctrl-generic",
    .supported_class = CLASS_PINCTRL,
    .match_compatible_id = "generic,pinctrl",
    .probe = pinctrl_probe,
    .remove = pinctrl_remove
};

int pinctrl_core_init(void) {
    return driver_register(&pinctrl_driver);
}

int pinctrl_controller_register(device_desc_t* dev) {
    if (!dev) return -1;
    dev->dev_class = CLASS_PINCTRL;
    return device_register(dev);
}