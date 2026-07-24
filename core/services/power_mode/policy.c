#include "services/power_mode/power_mode.h"

#include "services/telemetrymgr/thermal_policy.h"

int power_mode_apply_thermal_policy(const thermal_policy_state_t* state) {
    power_mode_thermal_state_t thermal;

    if (!state) {
        return -1;
    }

    thermal.max_temp_mc = 0;
    thermal.thermal_pressure_pct = state->global_pressure_pct;
    thermal.critical = state->severity >= THERMAL_SEVERITY_CRITICAL;

    return power_mode_set_thermal_state(&thermal);
}
