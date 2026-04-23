#include "pwm_core.h"
#include "../../core/device_registry.h"
#include "../../core/driver_registry.h"
#include <stddef.h>

static int pwm_probe(device_desc_t* dev) {
    if (!dev || dev->dev_class != CLASS_PWM) return -1;
    return 0;
}

static void pwm_remove(device_desc_t* dev) {
}

static driver_desc_t pwm_driver = {
    .name = "pwm-generic",
    .supported_class = CLASS_PWM,
    .match_compatible_id = "generic,pwm",
    .probe = pwm_probe,
    .remove = pwm_remove
};

int pwm_core_init(void) {
    return driver_register(&pwm_driver);
}

int pwm_controller_register(device_desc_t* dev) {
    if (!dev) return -1;
    dev->dev_class = CLASS_PWM;
    return device_register(dev);
}