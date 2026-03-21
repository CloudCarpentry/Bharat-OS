#include "drivers/motor/motor_control.h"
#include <stddef.h>

#define MAX_MOTORS 8

static motor_device_t* g_motors[MAX_MOTORS];
static int g_motor_count = 0;

int motor_core_register(motor_device_t* dev) {
    if (!dev || !dev->ops) return -1;

    for (int i = 0; i < g_motor_count; i++) {
        if (g_motors[i]->motor_id == dev->motor_id) return -1;
    }

    if (g_motor_count >= MAX_MOTORS) return -1;

    g_motors[g_motor_count++] = dev;
    return 0;
}

int motor_core_unregister(motor_device_t* dev) {
    if (!dev) return -1;

    for (int i = 0; i < g_motor_count; i++) {
        if (g_motors[i] == dev) {
            for (int j = i; j < g_motor_count - 1; j++) {
                g_motors[j] = g_motors[j + 1];
            }
            g_motor_count--;
            return 0;
        }
    }
    return -1;
}

motor_device_t* motor_core_get(uint32_t motor_id) {
    for (int i = 0; i < g_motor_count; i++) {
        if (g_motors[i]->motor_id == motor_id) {
            return g_motors[i];
        }
    }
    return NULL;
}
