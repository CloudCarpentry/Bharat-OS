#include "services/power_mode/power_mode.h"

static int thermal_guard_prepare(power_mode_state_t target) {
    power_mode_thermal_state_t thermal = power_mode_get_thermal_state();

    if (thermal.critical && target == POWER_MODE_RUN) {
        return -1;
    }

    return 0;
}

static int thermal_guard_commit(power_mode_state_t target) {
    (void)target;
    return 0;
}

static int thermal_guard_wake(power_mode_reason_t reason) {
    (void)reason;
    return 0;
}

int power_mode_register_default_clients(void) {
    return power_mode_register_client(thermal_guard_prepare, thermal_guard_commit, thermal_guard_wake);
}
