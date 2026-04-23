#include <stdint.h>
#include <stdbool.h>
#include <string.h>

/* This would typically come from our internal includes, but we'll mock the needed UAPI parts for now. */
#include "system/telemetry.h"

#define MAX_SUBSCRIPTIONS 32

typedef struct {
    bool active;
    uint32_t sub_id;
    uint32_t client_id;
    bharat_telemetry_filter_t filter;
} diag_subscription_t;

static diag_subscription_t s_subscriptions[MAX_SUBSCRIPTIONS];
static uint32_t s_next_sub_id = 1;

/**
 * Initialize the subscription subsystem.
 */
void diag_telemetry_init(void) {
    memset(s_subscriptions, 0, sizeof(s_subscriptions));
}

/**
 * Handle a Subscribe request.
 *
 * @param client_id ID of the subscribing client.
 * @param filter The filter parameters.
 * @param out_sub_id Pointer to store the assigned subscription ID.
 * @return 0 on success, negative error code on failure.
 */
int diag_telemetry_subscribe(uint32_t client_id, const bharat_telemetry_filter_t* filter, uint32_t* out_sub_id) {
    if (!filter || !out_sub_id) {
        return -1;
    }

    for (int i = 0; i < MAX_SUBSCRIPTIONS; ++i) {
        if (!s_subscriptions[i].active) {
            s_subscriptions[i].active = true;
            s_subscriptions[i].sub_id = s_next_sub_id++;
            s_subscriptions[i].client_id = client_id;
            s_subscriptions[i].filter = *filter;
            *out_sub_id = s_subscriptions[i].sub_id;
            return 0; // Success
        }
    }

    return -2; // Out of resources
}

/**
 * Handle an Unsubscribe request.
 *
 * @param sub_id The subscription ID to remove.
 * @return 0 on success, negative error code on failure.
 */
int diag_telemetry_unsubscribe(uint32_t sub_id) {
    for (int i = 0; i < MAX_SUBSCRIPTIONS; ++i) {
        if (s_subscriptions[i].active && s_subscriptions[i].sub_id == sub_id) {
            s_subscriptions[i].active = false;
            return 0; // Success
        }
    }
    return -1; // Not found
}

/**
 * Filter an incoming event against a subscription.
 *
 * @param event The event to check.
 * @param filter The filter to apply.
 * @return true if the event passes the filter, false otherwise.
 */
bool diag_telemetry_filter_event(const bharat_telemetry_event_t* event, const bharat_telemetry_filter_t* filter) {
    if (!event || !filter) {
        return false;
    }

    // Check kind mask
    if ((filter->event_kind_mask & (1 << event->kind)) == 0 && filter->event_kind_mask != 0xFFFFFFFF) {
        return false;
    }

    // Check severity
    if (event->severity < filter->min_severity) {
        return false;
    }

    // Check source
    if (filter->source_id != 0 && event->source_id != filter->source_id) {
        return false;
    }

    return true;
}

/**
 * Process an incoming event from the kernel or another subsystem.
 *
 * @param event The incoming event.
 */
void diag_telemetry_process_event(const bharat_telemetry_event_t* event) {
    if (!event) return;

    for (int i = 0; i < MAX_SUBSCRIPTIONS; ++i) {
        if (s_subscriptions[i].active) {
            if (diag_telemetry_filter_event(event, &s_subscriptions[i].filter)) {
                // In a real implementation, this would queue a message to the client_id via IPC
                // e.g., ipc_send_event(s_subscriptions[i].client_id, event);
            }
        }
    }
}