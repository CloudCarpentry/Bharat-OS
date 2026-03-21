#include "drivers/actuator/actuator_device.h"

// Simulate calling enter_safe_state on an actuator safely checking states
int actuator_mgr_force_safe_state(actuator_device_t* dev) {
    if (!dev || !dev->ops || !dev->ops->enter_safe_state) return -1;
    return dev->ops->enter_safe_state(dev);
}
