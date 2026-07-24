#include "i2c_registry.h"
#include <lib/base/string.h>

#define MAX_I2C_CONTROLLERS 16
static i2c_controller_t *g_controllers[MAX_I2C_CONTROLLERS] = {NULL};

void i2c_registry_init(void) {
    for (int i = 0; i < MAX_I2C_CONTROLLERS; i++) {
        g_controllers[i] = NULL;
    }
}

int i2c_controller_register(i2c_controller_t *ctrl) {
    if (!ctrl || !ctrl->ops || !ctrl->ops->master_xfer) {
        return -1; // Invalid arg
    }

    if (ctrl->bus_id < 0 || ctrl->bus_id >= MAX_I2C_CONTROLLERS) {
        return -1; // Out of bounds
    }

    if (g_controllers[ctrl->bus_id] != NULL) {
        return -1; // Already registered
    }

    g_controllers[ctrl->bus_id] = ctrl;
    return 0; // Success
}

void i2c_controller_unregister(i2c_controller_t *ctrl) {
    if (!ctrl || ctrl->bus_id < 0 || ctrl->bus_id >= MAX_I2C_CONTROLLERS) {
        return;
    }
    g_controllers[ctrl->bus_id] = NULL;
}

i2c_controller_t *i2c_get_controller(int bus_id) {
    if (bus_id < 0 || bus_id >= MAX_I2C_CONTROLLERS) {
        return NULL;
    }
    return g_controllers[bus_id];
}

// Stub implementations of device lifecycle
i2c_device_t *i2c_device_create(i2c_controller_t *ctrl, uint16_t addr, const char *name) {
    if (!ctrl || !name) return NULL;

    // In a real implementation this would use heap allocation via kalloc,
    // but for our structural mock we rely on static structures or minimal mocks
    // depending on architecture rules. We will return NULL for now to indicate
    // not fully wired memory allocator in this mock, but interface stands.
    return NULL;
}

void i2c_device_destroy(i2c_device_t *device) {
    (void)device;
    // Free the device memory once allocation is wired
}