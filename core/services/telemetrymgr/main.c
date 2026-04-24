#include <stdint.h>
#include <stdbool.h>

#include "services/telemetrymgr/thermal_policy.h"

/**
 * @file main.c
 * @brief telemetrymgr - System-wide metrics and monitoring.
 */

typedef struct {
    thermal_zone_reading_t zones[4];
    uint32_t zone_count;
    thermal_policy_state_t latest;
} telemetrymgr_state_t;

static telemetrymgr_state_t g_state;

static void telemetry_seed_zones(void) {
    g_state.zone_count = 2U;
    g_state.zones[0] = (thermal_zone_reading_t){
        .temp_mc = 73000,
        .passive_trip_mc = 70000,
        .hot_trip_mc = 85000,
        .critical_trip_mc = 95000,
    };
    g_state.zones[1] = (thermal_zone_reading_t){
        .temp_mc = 68000,
        .passive_trip_mc = 70000,
        .hot_trip_mc = 85000,
        .critical_trip_mc = 95000,
    };
}

void init_telemetrymgr(void) {
    telemetry_seed_zones();
    g_state.latest = thermal_policy_apply(g_state.zones, g_state.zone_count);
}

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;

    init_telemetrymgr();

    // Main event loop placeholder. Keep one iteration in hosted builds.
    while (true) {
        g_state.latest = thermal_policy_apply(g_state.zones, g_state.zone_count);
        break;
    }

    return (int)g_state.latest.severity;
}
