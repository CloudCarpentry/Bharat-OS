#include "event.h"
#include "bharat/uapi/sys_errno.h"
#include <stddef.h>

#define MAX_LISTENERS 8
static driver_event_cb_t g_listeners[MAX_LISTENERS];
static int g_listener_count = 0;
static driver_event_stats_t g_event_stats = {0};

int driver_event_register_listener(driver_event_cb_t cb) {
    /* Current contract: listener registration is boot-time or single-writer only.
     * Concurrent mutation requires adding locking/RCU/sequence-counter protection. */

    if (!cb) return -SYS_EINVAL;
    if (g_listener_count >= MAX_LISTENERS) return -SYS_ENOSPC;

    for (int i = 0; i < MAX_LISTENERS; i++) {
        if (g_listeners[i] == NULL) {
            g_listeners[i] = cb;
            g_listener_count++;
            return 0;
        }
    }
    return -SYS_ENOSPC;
}

void driver_event_emit(device_event_type_t type, device_desc_t* dev) {
    g_event_stats.emitted_count++;
    if (!dev) {
        g_event_stats.dropped_count++;
        return;
    }

    device_event_t evt;
    evt.type = type;
    evt.device = dev;

    bool delivered = false;
    for (int i = 0; i < MAX_LISTENERS; i++) {
        if (g_listeners[i]) {
            g_listeners[i](&evt);
            delivered = true;
        }
    }

    if (delivered) {
        g_event_stats.delivered_count++;
    } else {
        g_event_stats.dropped_count++;
    }
}

void driver_event_get_stats(driver_event_stats_t* stats) {
    if (stats) {
        *stats = g_event_stats;
    }
}