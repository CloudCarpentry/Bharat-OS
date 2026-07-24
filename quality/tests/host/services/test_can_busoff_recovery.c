#include <stdio.h>
#include <assert.h>
#include <stdbool.h>
#include "drivers/can/can_controller.h"

// Declaration to expose the internal virt_can functions for testing
int virt_can_register(void);
can_controller_t* can_controller_core_get(uint8_t controller_id);
void virt_can_force_bus_off(can_controller_t* ctrl);

void test_bus_off_recovery() {
    assert(virt_can_register() == 0);
    can_controller_t* ctrl = can_controller_core_get(0);
    assert(ctrl != NULL);

    ctrl->ops->start(ctrl);
    can_controller_state_t state;
    ctrl->ops->get_state(ctrl, &state);
    assert(state == CAN_CTRL_ERROR_ACTIVE);

    virt_can_force_bus_off(ctrl);
    ctrl->ops->get_state(ctrl, &state);
    assert(state == CAN_CTRL_BUS_OFF);

    assert(ctrl->ops->recover_bus(ctrl) == 0);
    ctrl->ops->get_state(ctrl, &state);
    assert(state == CAN_CTRL_ERROR_ACTIVE);

    can_controller_stats_t stats;
    ctrl->ops->get_stats(ctrl, &stats);
    assert(stats.bus_off_count == 1);

    can_controller_core_unregister(ctrl);
}

int main() {
    test_bus_off_recovery();
    printf("test_can_busoff_recovery passed.\n");
    return 0;
}
