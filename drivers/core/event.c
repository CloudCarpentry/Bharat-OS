#include "event.h"
#include <stddef.h>

#define MAX_LISTENERS 8
static driver_event_cb_t g_listeners[MAX_LISTENERS];
static int g_listener_count = 0;

int driver_event_register_listener(driver_event_cb_t cb) {
    if (!cb || g_listener_count >= MAX_LISTENERS) return -1;

    for (int i = 0; i < MAX_LISTENERS; i++) {
        if (g_listeners[i] == NULL) {
            g_listeners[i] = cb;
            g_listener_count++;
            return 0;
        }
    }
    return -1;
}

void driver_event_emit(device_event_type_t type, device_desc_t* dev) {
    if (!dev) return;

    device_event_t evt;
    evt.type = type;
    evt.device = dev;

    for (int i = 0; i < MAX_LISTENERS; i++) {
        if (g_listeners[i]) {
            g_listeners[i](&evt);
        }
    }
}