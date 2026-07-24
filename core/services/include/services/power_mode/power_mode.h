#pragma once
#include <stdbool.h>
#include <stdint.h>

#include "services/telemetrymgr/thermal_policy.h"

typedef enum {
    POWER_MODE_OFF = 0,
    POWER_MODE_ACCESSORY,
    POWER_MODE_RUN,
    POWER_MODE_CRANK,
    POWER_MODE_SLEEP_PREP,
    POWER_MODE_SLEEP,
    POWER_MODE_WAKE,
    POWER_MODE_LIMP_HOME
} power_mode_state_t;

typedef enum {
    POWER_REASON_NONE = 0,
    POWER_REASON_IGNITION,
    POWER_REASON_CAN_WAKE,
    POWER_REASON_TIMER_WAKE,
    POWER_REASON_FAULT_FORCED_LIMP,
    POWER_REASON_LOW_VOLTAGE,
    POWER_REASON_THERMAL_CRITICAL
} power_mode_reason_t;

typedef struct {
    power_mode_state_t from;
    power_mode_state_t to;
    power_mode_reason_t reason;
} power_mode_transition_t;

typedef struct {
    int32_t max_temp_mc;
    uint8_t thermal_pressure_pct;
    bool critical;
} power_mode_thermal_state_t;

typedef int (*power_mode_prepare_cb)(power_mode_state_t target);
typedef int (*power_mode_commit_cb)(power_mode_state_t target);
typedef int (*power_mode_wake_cb)(power_mode_reason_t reason);

int power_mode_register_client(power_mode_prepare_cb prepare, power_mode_commit_cb commit, power_mode_wake_cb wake);
int power_mode_request_transition(power_mode_state_t target, power_mode_reason_t reason);
int power_mode_force_limp_home(power_mode_reason_t reason);
power_mode_state_t power_mode_get_current(void);

int power_mode_set_thermal_state(const power_mode_thermal_state_t* thermal_state);
power_mode_thermal_state_t power_mode_get_thermal_state(void);

int power_mode_register_default_clients(void);
int power_mode_apply_thermal_policy(const thermal_policy_state_t* state);

// Internal function to reset state for testing
void power_mode_reset(void);
