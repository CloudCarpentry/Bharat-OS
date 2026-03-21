#include <stdio.h>
#include <assert.h>
#include "services/power_mode/power_mode.h"

// Expose internal reset for tests
void power_mode_reset(void);

void test_valid_transitions() {
    power_mode_reset();
    assert(power_mode_request_transition(POWER_MODE_ACCESSORY, POWER_REASON_IGNITION) == 0);
    assert(power_mode_get_current() == POWER_MODE_ACCESSORY);

    assert(power_mode_request_transition(POWER_MODE_RUN, POWER_REASON_IGNITION) == 0);
    assert(power_mode_get_current() == POWER_MODE_RUN);
}

void test_invalid_transition() {
    power_mode_reset();
    // Cannot go straight to SLEEP from OFF without RUN/SLEEP_PREP (or based on policy)
    assert(power_mode_request_transition(POWER_MODE_SLEEP, POWER_REASON_NONE) == -1);
}

int main() {
    test_valid_transitions();
    test_invalid_transition();
    printf("test_power_mode_state_machine passed.\n");
    return 0;
}
