#include <stdio.h>
#include <assert.h>
#include <bharat/uapi/service_status.h>
#include "services/power_mode/power_mode.h"

// Expose internal reset for tests
void power_mode_reset(void);

static bharat_status_t my_prepare_ok(power_mode_state_t t) { return BHARAT_STATUS_OK; }
static bharat_status_t my_prepare_reject(power_mode_state_t t) { return BHARAT_STATUS_ERR_INTERNAL; }

void test_callbacks_ok() {
    power_mode_reset();
    power_mode_register_client(my_prepare_ok, NULL, NULL);

    // valid path to sleep
    assert(power_mode_request_transition(POWER_MODE_RUN, POWER_REASON_IGNITION) == BHARAT_STATUS_OK);
    assert(power_mode_request_transition(POWER_MODE_SLEEP_PREP, POWER_REASON_IGNITION) == BHARAT_STATUS_OK);
}

void test_callbacks_reject() {
    power_mode_reset();
    power_mode_register_client(my_prepare_reject, NULL, NULL);

    // valid path to sleep prep
    assert(power_mode_request_transition(POWER_MODE_RUN, POWER_REASON_IGNITION) == BHARAT_STATUS_OK);
    // sleep prep should be rejected by client
    assert(power_mode_request_transition(POWER_MODE_SLEEP_PREP, POWER_REASON_IGNITION) == BHARAT_STATUS_ERR_INTERNAL);
}

int main() {
    test_callbacks_ok();
    test_callbacks_reject();
    printf("test_power_mode_callbacks passed.\n");
    return 0;
}
