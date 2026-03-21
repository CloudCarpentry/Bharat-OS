#include "services/diag/diag_event.h"
#include <stddef.h>

#define MAX_DTC_STORE 32

static diag_event_t g_store[MAX_DTC_STORE];
static int g_store_count = 0;

int diag_report_event(const diag_event_t* event) {
    if (!event) return -1;

    // Check if it already exists
    for (int i = 0; i < g_store_count; i++) {
        if (g_store[i].dtc == event->dtc) {
            // Update severity or timestamp if needed
            if (event->severity > g_store[i].severity) {
                g_store[i] = *event;
            }
            return 0;
        }
    }

    if (g_store_count >= MAX_DTC_STORE) {
        return -1; // Store full
    }

    g_store[g_store_count++] = *event;
    return 0;
}

int diag_clear_event(uint32_t dtc) {
    for (int i = 0; i < g_store_count; i++) {
        if (g_store[i].dtc == dtc) {
            for (int j = i; j < g_store_count - 1; j++) {
                g_store[j] = g_store[j + 1];
            }
            g_store_count--;
            return 0;
        }
    }
    return -1;
}

int diag_get_event(uint32_t dtc, diag_event_t* out_event) {
    if (!out_event) return -1;
    for (int i = 0; i < g_store_count; i++) {
        if (g_store[i].dtc == dtc) {
            *out_event = g_store[i];
            return 0;
        }
    }
    return -1;
}

void diag_store_reset(void) {
    g_store_count = 0;
}
