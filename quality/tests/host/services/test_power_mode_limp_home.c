#include <stdio.h>
#include <assert.h>
#include <stdbool.h>
#include "services/power_mode/power_mode.h"

// Expose internal reset for tests
void power_mode_reset(void);

static bool g_commit_called = false;
static power_mode_state_t g_last_commit_target = POWER_MODE_OFF;

static int mock_commit_cb(power_mode_state_t target) {
    g_commit_called = true;
    g_last_commit_target = target;
    return 0;
}

void test_limp_home_from_run() {
    power_mode_reset();
    assert(power_mode_request_transition(POWER_MODE_RUN, POWER_REASON_IGNITION) == 0);

    // Force limp home due to critical fault
    assert(power_mode_force_limp_home(POWER_REASON_FAULT_FORCED_LIMP) == 0);
    assert(power_mode_get_current() == POWER_MODE_LIMP_HOME);

    // Cannot transition to anything but OFF
    assert(power_mode_request_transition(POWER_MODE_RUN, POWER_REASON_NONE) == -1);
    assert(power_mode_request_transition(POWER_MODE_OFF, POWER_REASON_NONE) == 0);
    assert(power_mode_get_current() == POWER_MODE_OFF);
}

void test_limp_home_from_all_states() {
    power_mode_state_t states[] = {
        POWER_MODE_OFF,
        POWER_MODE_ACCESSORY,
        POWER_MODE_RUN,
        POWER_MODE_CRANK,
        POWER_MODE_SLEEP_PREP,
        POWER_MODE_SLEEP,
        POWER_MODE_WAKE,
        POWER_MODE_LIMP_HOME
    };

    for (int i = 0; i < sizeof(states)/sizeof(states[0]); i++) {
        power_mode_reset();

        // We might need to transition to the state first if we want to be thorough,
        // but power_mode_force_limp_home calls power_mode_request_transition which
        // checks is_valid_transition. is_valid_transition always returns true for target == POWER_MODE_LIMP_HOME.

        // For states like SLEEP, we need to get there first.
        if (states[i] == POWER_MODE_ACCESSORY) {
            assert(power_mode_request_transition(POWER_MODE_ACCESSORY, POWER_REASON_IGNITION) == 0);
        } else if (states[i] == POWER_MODE_RUN) {
            assert(power_mode_request_transition(POWER_MODE_RUN, POWER_REASON_IGNITION) == 0);
        } else if (states[i] == POWER_MODE_CRANK) {
            assert(power_mode_request_transition(POWER_MODE_RUN, POWER_REASON_IGNITION) == 0);
            assert(power_mode_request_transition(POWER_MODE_CRANK, POWER_REASON_IGNITION) == 0);
        } else if (states[i] == POWER_MODE_SLEEP_PREP) {
            assert(power_mode_request_transition(POWER_MODE_RUN, POWER_REASON_IGNITION) == 0);
            assert(power_mode_request_transition(POWER_MODE_SLEEP_PREP, POWER_REASON_NONE) == 0);
        } else if (states[i] == POWER_MODE_SLEEP) {
            assert(power_mode_request_transition(POWER_MODE_RUN, POWER_REASON_IGNITION) == 0);
            assert(power_mode_request_transition(POWER_MODE_SLEEP_PREP, POWER_REASON_NONE) == 0);
            assert(power_mode_request_transition(POWER_MODE_SLEEP, POWER_REASON_NONE) == 0);
        } else if (states[i] == POWER_MODE_WAKE) {
            assert(power_mode_request_transition(POWER_MODE_RUN, POWER_REASON_IGNITION) == 0);
            assert(power_mode_request_transition(POWER_MODE_SLEEP_PREP, POWER_REASON_NONE) == 0);
            assert(power_mode_request_transition(POWER_MODE_SLEEP, POWER_REASON_NONE) == 0);
            assert(power_mode_request_transition(POWER_MODE_WAKE, POWER_REASON_CAN_WAKE) == 0);
        } else if (states[i] == POWER_MODE_LIMP_HOME) {
            assert(power_mode_force_limp_home(POWER_REASON_FAULT_FORCED_LIMP) == 0);
        }

        assert(power_mode_get_current() == states[i]);

        // Now force limp home
        assert(power_mode_force_limp_home(POWER_REASON_FAULT_FORCED_LIMP) == 0);
        assert(power_mode_get_current() == POWER_MODE_LIMP_HOME);
    }
}

void test_limp_home_callbacks() {
    power_mode_reset();
    g_commit_called = false;
    power_mode_register_client(NULL, mock_commit_cb, NULL);

    assert(power_mode_request_transition(POWER_MODE_RUN, POWER_REASON_IGNITION) == 0);
    g_commit_called = false; // Reset for the next transition

    assert(power_mode_force_limp_home(POWER_REASON_FAULT_FORCED_LIMP) == 0);
    assert(g_commit_called == true);
    assert(g_last_commit_target == POWER_MODE_LIMP_HOME);
}

void test_thermal_critical_triggers_limp_home() {
    power_mode_reset();
    assert(power_mode_request_transition(POWER_MODE_RUN, POWER_REASON_IGNITION) == 0);

    power_mode_thermal_state_t thermal = {
        .max_temp_mc = 105000,
        .thermal_pressure_pct = 0,
        .critical = true
    };

    assert(power_mode_set_thermal_state(&thermal) == 0);
    assert(power_mode_get_current() == POWER_MODE_LIMP_HOME);
}

int main() {
    test_limp_home_from_run();
    test_limp_home_from_all_states();
    test_limp_home_callbacks();
    test_thermal_critical_triggers_limp_home();
    printf("test_power_mode_limp_home passed.\n");
    return 0;
}
