#include "power/cooling.h"
#include <stddef.h>

#define MAX_COOLING_DEVICES 32
#define MAX_COOLING_BINDINGS 64

static cooling_device_t *cooling_devices[MAX_COOLING_DEVICES];
static int num_cooling_devices = 0;

typedef struct {
    thermal_zone_t *tz;
    cooling_device_t *cdev;
    uint32_t trip_id;
} cooling_binding_t;

static cooling_binding_t cooling_bindings[MAX_COOLING_BINDINGS];
static int num_cooling_bindings = 0;

int cooling_device_register(cooling_device_t *cdev) {
    if (!cdev || !cdev->ops || !cdev->ops->get_max_state ||
        !cdev->ops->get_cur_state || !cdev->ops->set_cur_state) {
        return -1;
    }

    if (num_cooling_devices >= MAX_COOLING_DEVICES) {
        return -1;
    }

    cooling_devices[num_cooling_devices++] = cdev;
    return 0;
}

int cooling_device_bind_zone(thermal_zone_t *tz, cooling_device_t *cdev, uint32_t trip_id) {
    if (!tz || !cdev) return -1;

    if (num_cooling_bindings >= MAX_COOLING_BINDINGS) {
        return -1;
    }

    for (int i = 0; i < num_cooling_bindings; i++) {
        if (cooling_bindings[i].tz == tz &&
            cooling_bindings[i].cdev == cdev &&
            cooling_bindings[i].trip_id == trip_id) {
            return 0; /* Already bound */
        }
    }

    cooling_bindings[num_cooling_bindings].tz = tz;
    cooling_bindings[num_cooling_bindings].cdev = cdev;
    cooling_bindings[num_cooling_bindings].trip_id = trip_id;
    num_cooling_bindings++;

    return 0;
}

int cooling_device_unbind_zone(thermal_zone_t *tz, cooling_device_t *cdev, uint32_t trip_id) {
    if (!tz || !cdev) return -1;

    for (int i = 0; i < num_cooling_bindings; i++) {
        if (cooling_bindings[i].tz == tz &&
            cooling_bindings[i].cdev == cdev &&
            cooling_bindings[i].trip_id == trip_id) {

            /* Remove by swapping with the last element */
            cooling_bindings[i] = cooling_bindings[num_cooling_bindings - 1];
            num_cooling_bindings--;
            return 0;
        }
    }

    return -1; /* Binding not found */
}
