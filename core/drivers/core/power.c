#include "power.h"
#include "event.h"
#include "driver_registry.h"
#include "driver_core_internal.h"
#include <stddef.h>

// A simple helper to find a driver by compatible id for a device.
static driver_desc_t* find_driver_for_device(device_desc_t* dev) {
    if (!dev || !dev->compatible_id) return NULL;
    int capacity;
    driver_desc_t** drivers = driver_registry_get_all(&capacity);
    for (int i = 0; i < capacity; i++) {
        driver_desc_t* drv = drivers[i];
        if (drv && drv->match_compatible_id && driver_core_streq(dev->compatible_id, drv->match_compatible_id)) {
            return drv;
        }
    }
    return NULL;
}

int driver_power_suspend_device(device_desc_t* dev) {
    if (!dev) return -1;
    driver_desc_t* drv = find_driver_for_device(dev);
    if (drv && drv->suspend) {
        int ret = drv->suspend(dev);
        if (ret == 0) {
            driver_event_emit(EVENT_DEVICE_SUSPEND, dev);
        }
        return ret;
    }
    return -1; // Not supported
}

int driver_power_resume_device(device_desc_t* dev) {
    if (!dev) return -1;
    driver_desc_t* drv = find_driver_for_device(dev);
    if (drv && drv->resume) {
        int ret = drv->resume(dev);
        if (ret == 0) {
            driver_event_emit(EVENT_DEVICE_RESUME, dev);
        }
        return ret;
    }
    return -1; // Not supported
}