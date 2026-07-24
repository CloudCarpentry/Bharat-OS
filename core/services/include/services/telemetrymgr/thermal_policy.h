#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    THERMAL_SEVERITY_NORMAL = 0,
    THERMAL_SEVERITY_PASSIVE,
    THERMAL_SEVERITY_HOT,
    THERMAL_SEVERITY_CRITICAL
} thermal_severity_t;

typedef struct {
    int32_t temp_mc;
    int32_t passive_trip_mc;
    int32_t hot_trip_mc;
    int32_t critical_trip_mc;
} thermal_zone_reading_t;

typedef struct {
    thermal_severity_t severity;
    uint8_t global_pressure_pct;
    bool emergency_shutdown_recommended;
    uint32_t max_allowed_freq_pct;
    uint32_t scheduler_quota_pct;
} thermal_policy_state_t;

thermal_policy_state_t thermal_policy_apply(const thermal_zone_reading_t* zones, uint32_t zone_count);
