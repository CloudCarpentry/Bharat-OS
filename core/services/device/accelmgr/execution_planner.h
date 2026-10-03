#ifndef BHARAT_EXECUTION_PLANNER_H
#define BHARAT_EXECUTION_PLANNER_H

#include <bharat/uapi/exec/execution_intent.h>
#include <bharat/uapi/exec/execution_plan.h>
#include <bharat/uapi/system/execution_mode.h>
#include <uapi/capability/hw_caps.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Forward declarations for snapshot types
typedef struct {
    bool is_available;
    bool is_quarantined;
    bool capability_allowed;
} bh_accel_state_snapshot_t;

typedef struct {
    uint32_t power_level;
} bh_power_state_snapshot_t;

typedef struct {
    uint32_t temperature_c;
} bh_thermal_state_snapshot_t;

typedef struct {
    bool success;
    uint32_t status_code;
} bh_execution_plan_result_t;

// Pure deterministic planner function
bh_execution_plan_result_t
bh_execution_plan_build(
    const bh_execution_intent_v1_t *intent,
    const bharat_execution_config_t *execution_config,
    const bharat_hw_caps_t *hw_caps,
    const bh_accel_state_snapshot_t *accel,
    const bh_power_state_snapshot_t *power,
    const bh_thermal_state_snapshot_t *thermal,
    bh_execution_plan_v1_t *out);

#ifdef __cplusplus
}
#endif

#endif // BHARAT_EXECUTION_PLANNER_H
