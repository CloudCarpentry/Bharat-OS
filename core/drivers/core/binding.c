#include "binding.h"
#include "bharat/uapi/sys_errno.h"
#include <stddef.h>

#define MAX_BINDINGS 128
static device_binding_t g_bindings[MAX_BINDINGS];
static int g_binding_count = 0;
static uint32_t g_next_binding_id = 1;

int device_binding_registry_init(void) {
    g_binding_count = 0;
    g_next_binding_id = 1;
    for (int i = 0; i < MAX_BINDINGS; i++) {
        g_bindings[i].binding_id = 0;
        g_bindings[i].device = NULL;
        g_bindings[i].driver = NULL;
    }
    return 0;
}

device_binding_t* device_binding_create(device_desc_t* dev, driver_desc_t* drv, int score) {
    if (!dev || !drv || g_binding_count >= MAX_BINDINGS) return NULL;

    for (int i = 0; i < MAX_BINDINGS; i++) {
        if (g_bindings[i].binding_id == 0) {
            g_bindings[i].binding_id = g_next_binding_id++;
            g_bindings[i].device = dev;
            g_bindings[i].driver = drv;
            g_bindings[i].state = DRIVER_STATE_MATCHED;
            g_bindings[i].match_score = score;
            g_bindings[i].match_priority = (uint32_t)drv->priority;
            g_bindings[i].driver_ctx = NULL;
            g_binding_count++;

            dev->driver_data = &g_bindings[i]; // Link for now
            return &g_bindings[i];
        }
    }
    return NULL;
}

device_binding_t* device_binding_find_by_dev(device_desc_t* dev) {
    if (!dev) return NULL;
    for (int i = 0; i < MAX_BINDINGS; i++) {
        if (g_bindings[i].binding_id != 0 && g_bindings[i].device == dev) {
            return &g_bindings[i];
        }
    }
    return NULL;
}

int device_binding_probe(device_binding_t* binding) {
    if (!binding) return -SYS_EINVAL;

    if (binding->state != DRIVER_STATE_MATCHED && binding->state != DRIVER_STATE_FAILED) {
        return -SYS_EPERM;
    }

    if (binding->driver->probe) {
        int ret = binding->driver->probe(binding->device);
        if (ret != 0) {
            binding->state = DRIVER_STATE_FAILED;
            return ret;
        }
    }

    binding->state = DRIVER_STATE_PROBED;
    return 0;
}

int device_binding_start(device_binding_t* binding) {
    if (!binding) return -SYS_EINVAL;

    if (binding->state != DRIVER_STATE_PROBED && binding->state != DRIVER_STATE_STOPPED) {
        return -SYS_EPERM;
    }

    binding->state = DRIVER_STATE_STARTED;
    return 0;
}

int device_binding_stop(device_binding_t* binding) {
    if (!binding) return -SYS_EINVAL;

    if (binding->state != DRIVER_STATE_STARTED) {
        return -SYS_EPERM;
    }

    binding->state = DRIVER_STATE_STOPPED;
    return 0;
}

int device_binding_remove(device_binding_t* binding) {
    if (!binding) return -SYS_EINVAL;

    if (binding->state == DRIVER_STATE_REMOVED) {
        return -SYS_EPERM;
    }

    if (binding->driver->remove) {
        binding->driver->remove(binding->device);
    }

    binding->state = DRIVER_STATE_REMOVED;

    // Clear the binding to allow slot reuse in registry
    binding->binding_id = 0;
    binding->device->driver_data = NULL;
    binding->device = NULL;
    binding->driver = NULL;
    g_binding_count--;

    return 0;
}

void device_binding_fail(device_binding_t* binding) {
    if (!binding) return;
    binding->state = DRIVER_STATE_FAILED;
}
