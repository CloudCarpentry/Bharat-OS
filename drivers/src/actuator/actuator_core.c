#include "drivers/actuator/actuator_device.h"
#include <stddef.h>
#include <math.h>

#define MAX_ACTUATORS 32

static actuator_device_t* g_actuators[MAX_ACTUATORS];
static int g_actuator_count = 0;

int actuator_core_register(actuator_device_t* dev) {
    if (!dev || !dev->ops) return -1;

    for (int i = 0; i < g_actuator_count; i++) {
        if (g_actuators[i]->actuator_id == dev->actuator_id) return -1;
    }

    if (g_actuator_count >= MAX_ACTUATORS) return -1;

    g_actuators[g_actuator_count++] = dev;
    return 0;
}

int actuator_core_unregister(actuator_device_t* dev) {
    if (!dev) return -1;

    for (int i = 0; i < g_actuator_count; i++) {
        if (g_actuators[i] == dev) {
            for (int j = i; j < g_actuator_count - 1; j++) {
                g_actuators[j] = g_actuators[j + 1];
            }
            g_actuator_count--;
            return 0;
        }
    }
    return -1;
}

actuator_device_t* actuator_core_get(uint32_t actuator_id) {
    for (int i = 0; i < g_actuator_count; i++) {
        if (g_actuators[i]->actuator_id == actuator_id) {
            return g_actuators[i];
        }
    }
    return NULL;
}
