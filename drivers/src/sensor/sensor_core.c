#include "drivers/sensor/sensor_device.h"
#include <stddef.h>

#define MAX_SENSORS 32

static sensor_device_t* g_sensors[MAX_SENSORS];
static int g_sensor_count = 0;

int sensor_core_register(sensor_device_t* dev) {
    if (!dev || !dev->ops) return -1;

    for (int i = 0; i < g_sensor_count; i++) {
        if (g_sensors[i]->sensor_id == dev->sensor_id) return -1; // duplicate
    }

    if (g_sensor_count >= MAX_SENSORS) return -1;

    g_sensors[g_sensor_count++] = dev;
    return 0;
}

int sensor_core_unregister(sensor_device_t* dev) {
    if (!dev) return -1;

    for (int i = 0; i < g_sensor_count; i++) {
        if (g_sensors[i] == dev) {
            for (int j = i; j < g_sensor_count - 1; j++) {
                g_sensors[j] = g_sensors[j + 1];
            }
            g_sensor_count--;
            return 0;
        }
    }
    return -1;
}

sensor_device_t* sensor_core_get(uint32_t sensor_id) {
    for (int i = 0; i < g_sensor_count; i++) {
        if (g_sensors[i]->sensor_id == sensor_id) {
            return g_sensors[i];
        }
    }
    return NULL;
}
