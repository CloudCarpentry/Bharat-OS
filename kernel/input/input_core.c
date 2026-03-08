#include "bharat/input/input.h"
#include <string.h>

#define BHARAT_MAX_INPUT_DEVS 8

static bharat_input_device_t *g_input_devs[BHARAT_MAX_INPUT_DEVS];
static uint32_t g_num_input_devs = 0;

/**
 * Register an input device.
 */
int bharat_input_register(bharat_input_device_t *dev) {
    if (!dev || !dev->ops) {
        return -1;
    }

    if (g_num_input_devs >= BHARAT_MAX_INPUT_DEVS) {
        return -2;
    }

    dev->id = g_num_input_devs;
    g_input_devs[g_num_input_devs++] = dev;

    if (dev->ops->open) {
        dev->ops->open(dev);
    }

    return 0;
}

/**
 * Receive events from hardware drivers and route them.
 * A real implementation would queue these and wake up listeners/UI.
 */
void bharat_input_report_event(bharat_input_device_t *dev, uint16_t type, uint16_t code, int32_t value) {
    if (!dev) return;

    bharat_input_event_t ev;
    // timestamp_sec and usec would come from real kernel clocks
    ev.timestamp_sec = 0;
    ev.timestamp_usec = 0;
    ev.type = type;
    ev.code = code;
    ev.value = value;

    // TODO: Route `ev` to a ring buffer consumed by `fbui/fb_events.c` or a Wayland-like compositor.
    // printf("Event: %d code %d value %d from %s\n", type, code, value, dev->name);
}

/**
 * Sync flush after a block of events.
 */
void bharat_input_sync(bharat_input_device_t *dev) {
    bharat_input_report_event(dev, EV_SYN, 0, 0); // EV_SYN == 0
}
