#include "drm_registry.h"
#include <stddef.h>

#define MAX_DRM_DEVICES 4

static drm_device_t *g_drm_devices[MAX_DRM_DEVICES] = {NULL};

void drm_registry_init(void) {
    for (int i = 0; i < MAX_DRM_DEVICES; i++) {
        g_drm_devices[i] = NULL;
    }
}

int drm_device_register(drm_device_t *dev) {
    if (!dev || !dev->ops) {
        return -1; // Invalid argument
    }

    if (dev->id < 0 || dev->id >= MAX_DRM_DEVICES) {
        return -1; // Out of bounds
    }

    if (g_drm_devices[dev->id] != NULL) {
        return -1; // Already registered
    }

    if (dev->ops->load) {
        int ret = dev->ops->load(dev);
        if (ret != 0) return ret;
    }

    g_drm_devices[dev->id] = dev;
    return 0; // Success
}

void drm_device_unregister(drm_device_t *dev) {
    if (!dev || dev->id < 0 || dev->id >= MAX_DRM_DEVICES) {
        return;
    }

    if (g_drm_devices[dev->id] == dev) {
        if (dev->ops->unload) {
            dev->ops->unload(dev);
        }
        g_drm_devices[dev->id] = NULL;
    }
}

drm_device_t *drm_get_device(int id) {
    if (id < 0 || id >= MAX_DRM_DEVICES) {
        return NULL;
    }
    return g_drm_devices[id];
}