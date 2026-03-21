#include <stdio.h>
#include <assert.h>
#include "drivers/actuator/actuator_device.h"

int actuator_mgr_force_safe_state(actuator_device_t* dev);

static actuator_state_t g_state = ACTUATOR_STATE_ARMED;

int mock_enter_safe(actuator_device_t* dev) {
    g_state = ACTUATOR_STATE_SAFE;
    return 0;
}

void test_safe_state() {
    g_state = ACTUATOR_STATE_ARMED;
    actuator_device_ops_t ops = { .enter_safe_state = mock_enter_safe };
    actuator_device_t dev = { .ops = &ops };

    assert(actuator_mgr_force_safe_state(&dev) == 0);
    assert(g_state == ACTUATOR_STATE_SAFE);
}

int main() {
    test_safe_state();
    printf("test_actuator_safe_state passed.\n");
    return 0;
}
