#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "services/telemetrymgr/thermal_policy.h"
#include <bharat/service/service_runtime.h>
#include <bharat/runtime/runtime.h>

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

bharat_status_t bh_service_handle_msg(bh_service_ctx_t *ctx, const bh_msg_t *msg) {
    (void)ctx;
    // Handle telemetry recording, metrics query, and supervisor lifecycle events land here.
    bharat_runtime_log("telemetrymgr: received message (opcode: %d)", msg->header.opcode);

    // Process thermal policy on each message for now
    g_state.latest = thermal_policy_apply(g_state.zones, g_state.zone_count);

    return BHARAT_STATUS_OK;
}

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;

    bharat_runtime_init();
    init_telemetrymgr();

    bh_service_start_info_t info = {
        .service_id = 0x00010008,
        .service_name = "telemetrymgr",
        .endpoint = BHARAT_CAP_INVALID_HANDLE
    };

    return bh_service_main(&info);
}
