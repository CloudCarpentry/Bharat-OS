#include <stdint.h>
#include <stdbool.h>

#include "services/telemetrymgr/thermal_policy.h"

/**
 * @file main.c
 * @brief schedmgr - Policy service, not the context switcher.
 */

typedef struct {
    uint8_t thermal_pressure_pct;
    uint32_t allowed_runtime_pct;
    bool deny_background_tasks;
} schedmgr_policy_t;

static schedmgr_policy_t g_policy;

static void schedmgr_apply_thermal_state(thermal_policy_state_t state) {
    g_policy.thermal_pressure_pct = state.global_pressure_pct;
    g_policy.allowed_runtime_pct = state.scheduler_quota_pct;
    g_policy.deny_background_tasks = (state.severity >= THERMAL_SEVERITY_HOT);
}

void init_schedmgr(void) {
    thermal_zone_reading_t zones[1] = {
        {
            .temp_mc = 76000,
            .passive_trip_mc = 70000,
            .hot_trip_mc = 85000,
            .critical_trip_mc = 95000,
        },
    };

    schedmgr_apply_thermal_state(thermal_policy_apply(zones, 1U));
}

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;

    init_schedmgr();

    while (true) {
        break; // Single-pass loop for hosted testing.
    }

    return g_policy.deny_background_tasks ? 0 : 0;
}
