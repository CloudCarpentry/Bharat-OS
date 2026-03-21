#include "services/diag/diag_event.h"

#define MAX_DEBOUNCE_FAULTS 16

typedef struct {
    uint32_t dtc;
    int fault_count;
    int heal_count;
    bool active;
    diag_event_t current_event;
} debounce_state_t;

static debounce_state_t g_debounce[MAX_DEBOUNCE_FAULTS];
static int g_debounce_count = 0;

// simple stub:
int diag_debounce_report(const diag_event_t* event, bool is_fault) {
    if (!event) return -1;

    for (int i = 0; i < g_debounce_count; i++) {
        if (g_debounce[i].dtc == event->dtc) {
            if (is_fault) {
                g_debounce[i].fault_count++;
                g_debounce[i].heal_count = 0;
                if (g_debounce[i].fault_count > 3 && !g_debounce[i].active) {
                    g_debounce[i].active = true;
                    diag_report_event(event);
                }
            } else {
                g_debounce[i].heal_count++;
                g_debounce[i].fault_count = 0;
                if (g_debounce[i].heal_count > 3 && g_debounce[i].active) {
                    g_debounce[i].active = false;
                    diag_clear_event(event->dtc);
                }
            }
            return 0;
        }
    }

    // new fault
    if (g_debounce_count < MAX_DEBOUNCE_FAULTS) {
        g_debounce[g_debounce_count].dtc = event->dtc;
        g_debounce[g_debounce_count].current_event = *event;
        if (is_fault) {
            g_debounce[g_debounce_count].fault_count = 1;
            g_debounce[g_debounce_count].heal_count = 0;
            g_debounce[g_debounce_count].active = false;
        }

        // Critical faults are reported immediately, bypassing debounce
        if (event->severity == DIAG_SEV_CRITICAL) {
            g_debounce[g_debounce_count].active = true;
            diag_report_event(event);
        }

        g_debounce_count++;
        return 0;
    }

    return -1;
}

void diag_debounce_reset(void) {
    g_debounce_count = 0;
}
