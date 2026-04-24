#include "device_registry.h"
#include "event.h"
#include "match.h"
#include <stddef.h>

static int _strcmp(const char *s1, const char *s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(const unsigned char *)s1 - *(const unsigned char *)s2;
}

// Extremely minimal skeleton, using a fixed size array for simplicity in this baseline PR.
#define MAX_DEVICES 128
static device_desc_t* g_devices[MAX_DEVICES];
static int g_device_count = 0;

int device_registry_init(void) {
    g_device_count = 0;
    for(int i = 0; i < MAX_DEVICES; i++) {
        g_devices[i] = NULL;
    }
    return 0;
}

int device_register(device_desc_t* dev) {
    if (!dev || g_device_count >= MAX_DEVICES) return -1;

    // Add to registry
    for (int i = 0; i < MAX_DEVICES; i++) {
        if (g_devices[i] == NULL) {
            g_devices[i] = dev;
            g_device_count++;

            // Emit event
            driver_event_emit(EVENT_DEVICE_ADDED, dev);

            // Attempt match
            driver_match_device(dev);
            return 0;
        }
    }
    return -1;
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
        if (g_devices[i] && g_devices[i]->name && _strcmp(g_devices[i]->name, name) == 0) {
            return g_devices[i];
        }
    }
    return NULL;
}