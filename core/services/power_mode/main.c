#include "services/power_mode/power_mode.h"


int main(void) {
    thermal_policy_state_t state;

    (void)power_mode_register_default_clients();

    state = (thermal_policy_state_t){
        .severity = THERMAL_SEVERITY_NORMAL,
        .global_pressure_pct = 10U,
        .emergency_shutdown_recommended = false,
        .max_allowed_freq_pct = 100U,
        .scheduler_quota_pct = 100U,
    };

    (void)power_mode_apply_thermal_policy(&state);
    (void)power_mode_request_transition(POWER_MODE_RUN, POWER_REASON_IGNITION);

    return 0;
}
