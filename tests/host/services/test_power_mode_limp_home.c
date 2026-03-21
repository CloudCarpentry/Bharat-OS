#include <stdio.h>
#include <assert.h>
#include "services/power_mode/power_mode.h"

// Expose internal reset for tests
void power_mode_reset(void);

void test_limp_home() {
    power_mode_reset();
    assert(power_mode_request_transition(POWER_MODE_RUN, POWER_REASON_IGNITION) == 0);

    // Force limp home due to critical fault
    assert(power_mode_force_limp_home(POWER_REASON_FAULT_FORCED_LIMP) == 0);
    assert(power_mode_get_current() == POWER_MODE_LIMP_HOME);

    // Cannot transition to anything but OFF
    assert(power_mode_request_transition(POWER_MODE_RUN, POWER_REASON_NONE) == -1);
    assert(power_mode_request_transition(POWER_MODE_OFF, POWER_REASON_NONE) == 0);
}

int main() {
    test_limp_home();
    printf("test_power_mode_limp_home passed.\n");
    return 0;
}
