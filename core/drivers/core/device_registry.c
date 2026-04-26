#include "device_registry.h"
#include "event.h"
#include "match.h"
#include "binding.h"
#include "driver_core_internal.h"
#include "bharat/uapi/sys_errno.h"
#include <stddef.h>

// Extremely minimal skeleton, using a fixed size array for simplicity in this baseline PR.
#define MAX_DEVICES 128
static device_desc_t* g_devices[MAX_DEVICES];
static int g_device_count = 0;
static uint32_t g_next_device_registry_id = 1;

int device_registry_init(void) {
    g_device_count = 0;
    g_next_device_registry_id = 1;
    device_binding_registry_init();
    for(int i = 0; i < MAX_DEVICES; i++) {
        g_devices[i] = NULL;
    }
    return 0;
}

int device_register(device_desc_t* dev) {
    /* Current contract: driver/device registration is boot-time or single-writer only.
     * Runtime hotplug requires adding locking/RCU/sequence-counter protection
     * before concurrent mutation is enabled. */

    if (!dev) return -SYS_EINVAL;
    if (!driver_core_name_valid(dev->name)) return -SYS_EINVAL;

    if (g_device_count >= MAX_DEVICES) return -SYS_ENOSPC;

    // Check for duplicates
    if (device_find_by_name(dev->name)) return -SYS_EEXIST;

    // Add to registry
    for (int i = 0; i < MAX_DEVICES; i++) {
        if (g_devices[i] == NULL) {
            dev->device_registry_id = g_next_device_registry_id++;
            g_devices[i] = dev;
            g_device_count++;

            // Emit event
            driver_event_emit(EVENT_DEVICE_ADDED, dev);

            // Attempt match
            driver_match_device(dev);
            return 0;
        }
    }
    return -SYS_ENOSPC;
}

void device_unregister(device_desc_t* dev) {
    if (!dev) return;

    for (int i = 0; i < MAX_DEVICES; i++) {
        if (g_devices[i] == dev) {
            g_devices[i] = NULL;
            g_device_count--;

            driver_event_emit(EVENT_DEVICE_REMOVED, dev);
            return;
        }
    }
}

device_desc_t* device_find_by_name(const char* name) {
    if (!name) return NULL;
    for (int i = 0; i < MAX_DEVICES; i++) {
        if (g_devices[i] && driver_core_streq(g_devices[i]->name, name)) {
            return g_devices[i];
        }
    }
    return NULL;
}

int device_registry_get_count(void) {
    return g_device_count;
}
