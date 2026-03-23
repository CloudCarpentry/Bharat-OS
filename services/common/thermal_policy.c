#include "services/telemetrymgr/thermal_policy.h"

static uint8_t clamp_pct(uint32_t val) {
    return (uint8_t)(val > 100U ? 100U : val);
}

thermal_policy_state_t thermal_policy_apply(const thermal_zone_reading_t* zones, uint32_t zone_count) {
    thermal_policy_state_t state = {
        .severity = THERMAL_SEVERITY_NORMAL,
        .global_pressure_pct = 0U,
        .emergency_shutdown_recommended = false,
        .max_allowed_freq_pct = 100U,
        .scheduler_quota_pct = 100U,
    };

    if (!zones || zone_count == 0U) {
        return state;
    }

    for (uint32_t i = 0U; i < zone_count; ++i) {
        const thermal_zone_reading_t* zone = &zones[i];

        if (zone->temp_mc >= zone->critical_trip_mc) {
            state.severity = THERMAL_SEVERITY_CRITICAL;
            state.global_pressure_pct = 100U;
            break;
        }
        if (zone->temp_mc >= zone->hot_trip_mc) {
            if (state.severity < THERMAL_SEVERITY_HOT) {
                state.severity = THERMAL_SEVERITY_HOT;
            }
            if (state.global_pressure_pct < 85U) {
                state.global_pressure_pct = 85U;
            }
            continue;
        }
        if (zone->temp_mc >= zone->passive_trip_mc) {
            if (state.severity < THERMAL_SEVERITY_PASSIVE) {
                state.severity = THERMAL_SEVERITY_PASSIVE;
            }
            if (state.global_pressure_pct < 50U) {
                state.global_pressure_pct = 50U;
            }
        }
    }

    switch (state.severity) {
        case THERMAL_SEVERITY_NORMAL:
            state.max_allowed_freq_pct = 100U;
            state.scheduler_quota_pct = 100U;
            break;
        case THERMAL_SEVERITY_PASSIVE:
            state.max_allowed_freq_pct = 80U;
            state.scheduler_quota_pct = 85U;
            break;
        case THERMAL_SEVERITY_HOT:
            state.max_allowed_freq_pct = 60U;
            state.scheduler_quota_pct = 65U;
            break;
        case THERMAL_SEVERITY_CRITICAL:
            state.max_allowed_freq_pct = 35U;
            state.scheduler_quota_pct = 40U;
            state.emergency_shutdown_recommended = true;
            break;
        default:
            break;
    }

    state.global_pressure_pct = clamp_pct(state.global_pressure_pct);
    return state;
}
