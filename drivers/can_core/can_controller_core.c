#include "bharat/drivers/can_controller.h"
#include <string.h>

#define MAX_CAN_CONTROLLERS 8

static can_controller_t* g_can_controllers[MAX_CAN_CONTROLLERS];
static int g_num_controllers = 0;

int can_controller_core_register(can_controller_t* ctrl) {
    if (!ctrl || !ctrl->ops) {
        return -1; // Invalid arguments
    }

    if (g_num_controllers >= MAX_CAN_CONTROLLERS) {
        return -1; // Max controllers reached
    }

    g_can_controllers[g_num_controllers++] = ctrl;

    if (ctrl->ops->init) {
        return ctrl->ops->init(ctrl);
    }

    return 0;
}

int can_controller_core_unregister(can_controller_t* ctrl) {
    if (!ctrl) {
        return -1;
    }

    for (int i = 0; i < g_num_controllers; i++) {
        if (g_can_controllers[i] == ctrl) {
            // Shift elements down
            for (int j = i; j < g_num_controllers - 1; j++) {
                g_can_controllers[j] = g_can_controllers[j + 1];
            }
            g_can_controllers[g_num_controllers - 1] = NULL;
            g_num_controllers--;

            if (ctrl->ops->stop) {
                ctrl->ops->stop(ctrl);
            }

            return 0;
        }
    }

    return -1; // Controller not found
}

// These are hooks for architecture-specific interrupt handlers to call
void can_controller_handle_rx_irq(can_controller_t* ctrl, const can_frame_t* frame) {
    if (!ctrl || !frame) return;

    // In a real RTOS, we would push this frame into a thread-safe IPC ring buffer
    // or wake up a worker thread to process it and dispatch to subscribed clients.
    // For now, this is a stub.
}

void can_controller_handle_tx_irq(can_controller_t* ctrl) {
    if (!ctrl) return;

    // In a real RTOS, we would check the software Tx queue and dequeue the next
    // frame to send, then call ctrl->ops->send().
    // For now, this is a stub.
}

void can_controller_handle_error_irq(can_controller_t* ctrl, can_controller_state_t new_state) {
    if (!ctrl) return;

    ctrl->state = new_state;

    if (new_state == CAN_STATE_BUS_OFF) {
        ctrl->stats.bus_off_count++;
        // Trigger recovery policy if configured, or notify userspace CAN service
    }
}
