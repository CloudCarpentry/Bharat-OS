#include "drivers/can/can_controller.h"

#define MAX_CAN_CONTROLLERS 8

static can_controller_t* g_can_controllers[MAX_CAN_CONTROLLERS] = {0};

int can_controller_core_register(can_controller_t* ctrl) {
    if (!ctrl || !ctrl->ops) {
        return -1;
    }

    if (ctrl->controller_id >= MAX_CAN_CONTROLLERS) {
        return -1;
    }

    if (g_can_controllers[ctrl->controller_id]) {
        return -1; // Already registered
    }

    g_can_controllers[ctrl->controller_id] = ctrl;
    return 0;
}

int can_controller_core_unregister(can_controller_t* ctrl) {
    if (!ctrl || ctrl->controller_id >= MAX_CAN_CONTROLLERS) {
        return -1;
    }

    if (g_can_controllers[ctrl->controller_id] != ctrl) {
        return -1;
    }

    g_can_controllers[ctrl->controller_id] = NULL;
    return 0;
}

can_controller_t* can_controller_core_get(uint8_t controller_id) {
    if (controller_id >= MAX_CAN_CONTROLLERS) {
        return NULL;
    }
    return g_can_controllers[controller_id];
}
