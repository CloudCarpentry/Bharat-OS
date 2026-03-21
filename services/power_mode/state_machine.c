#include "services/power_mode/power_mode.h"
#include <stdio.h>
#include <stddef.h>

#define MAX_PM_CLIENTS 16

typedef struct {
    power_mode_prepare_cb prepare;
    power_mode_commit_cb commit;
    power_mode_wake_cb wake;
} pm_client_t;

static pm_client_t g_clients[MAX_PM_CLIENTS] = {0};
static int g_num_clients = 0;
static power_mode_state_t g_current_mode = POWER_MODE_OFF;

int power_mode_register_client(power_mode_prepare_cb prepare, power_mode_commit_cb commit, power_mode_wake_cb wake) {
    if (g_num_clients >= MAX_PM_CLIENTS) {
        return -1;
    }
    g_clients[g_num_clients].prepare = prepare;
    g_clients[g_num_clients].commit = commit;
    g_clients[g_num_clients].wake = wake;
    g_num_clients++;
    return 0;
}

static bool is_valid_transition(power_mode_state_t current, power_mode_state_t target) {
    if (target == POWER_MODE_LIMP_HOME) return true; // Always allow entering limp home

    switch (current) {
        case POWER_MODE_OFF:
            return target == POWER_MODE_ACCESSORY || target == POWER_MODE_RUN || target == POWER_MODE_WAKE;
        case POWER_MODE_ACCESSORY:
            return target == POWER_MODE_OFF || target == POWER_MODE_RUN;
        case POWER_MODE_RUN:
            return target == POWER_MODE_OFF || target == POWER_MODE_CRANK || target == POWER_MODE_SLEEP_PREP || target == POWER_MODE_ACCESSORY;
        case POWER_MODE_SLEEP_PREP:
            return target == POWER_MODE_SLEEP || target == POWER_MODE_RUN; // RUN if aborting sleep
        case POWER_MODE_SLEEP:
            return target == POWER_MODE_WAKE;
        case POWER_MODE_WAKE:
            return target == POWER_MODE_ACCESSORY || target == POWER_MODE_RUN;
        case POWER_MODE_LIMP_HOME:
            return target == POWER_MODE_OFF; // Only exit limp home to OFF
        case POWER_MODE_CRANK:
            return target == POWER_MODE_RUN || target == POWER_MODE_OFF;
        default:
            return false;
    }
}

int power_mode_request_transition(power_mode_state_t target, power_mode_reason_t reason) {
    (void)reason;
    if (!is_valid_transition(g_current_mode, target)) {
        return -1;
    }

    if (target == POWER_MODE_SLEEP || target == POWER_MODE_SLEEP_PREP) {
        // Prepare phase
        for (int i = 0; i < g_num_clients; i++) {
            if (g_clients[i].prepare) {
                if (g_clients[i].prepare(target) != 0) {
                    return -1; // Client rejected sleep prep
                }
            }
        }
    }

    // Commit phase
    for (int i = 0; i < g_num_clients; i++) {
        if (g_clients[i].commit) {
            g_clients[i].commit(target);
        }
    }

    if (target == POWER_MODE_WAKE) {
        for (int i = 0; i < g_num_clients; i++) {
            if (g_clients[i].wake) {
                g_clients[i].wake(reason);
            }
        }
    }

    g_current_mode = target;
    return 0;
}

int power_mode_force_limp_home(power_mode_reason_t reason) {
    return power_mode_request_transition(POWER_MODE_LIMP_HOME, reason);
}

power_mode_state_t power_mode_get_current(void) {
    return g_current_mode;
}

// Internal function to reset state for testing
void power_mode_reset(void) {
    g_num_clients = 0;
    g_current_mode = POWER_MODE_OFF;
}
