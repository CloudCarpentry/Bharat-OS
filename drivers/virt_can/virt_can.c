#include "drivers/can/can_controller.h"
#include <string.h>

#define VIRT_CAN_QUEUE_SIZE 128

typedef struct {
    can_frame_t rx_queue[VIRT_CAN_QUEUE_SIZE];
    uint32_t head;
    uint32_t tail;
    uint32_t count;
    bool loopback_enabled;
    can_controller_state_t state;
    can_controller_stats_t stats;
} virt_can_priv_t;

static virt_can_priv_t g_virt_can_priv;
static can_controller_t g_virt_can_ctrl;

static int virt_can_start(can_controller_t* ctrl) {
    virt_can_priv_t* priv = (virt_can_priv_t*)ctrl->priv;
    if (priv->state != CAN_CTRL_STOPPED) {
        return -1;
    }
    priv->state = CAN_CTRL_ERROR_ACTIVE;
    return 0;
}

static int virt_can_stop(can_controller_t* ctrl) {
    virt_can_priv_t* priv = (virt_can_priv_t*)ctrl->priv;
    priv->state = CAN_CTRL_STOPPED;
    return 0;
}

static int virt_can_transmit(can_controller_t* ctrl, const can_frame_t* frame) {
    virt_can_priv_t* priv = (virt_can_priv_t*)ctrl->priv;

    if (priv->state == CAN_CTRL_STOPPED || priv->state == CAN_CTRL_BUS_OFF) {
        return -1;
    }

    priv->stats.tx_frames++;

    if (priv->loopback_enabled) {
        if (priv->count >= VIRT_CAN_QUEUE_SIZE) {
            priv->stats.tx_drops++;
            return -1;
        }

        priv->rx_queue[priv->tail] = *frame;
        priv->tail = (priv->tail + 1) % VIRT_CAN_QUEUE_SIZE;
        priv->count++;
    }

    return 0;
}

static int virt_can_set_filters(can_controller_t* ctrl, const can_filter_t* filters, size_t count) {
    (void)ctrl; (void)filters; (void)count;
    // Virt CAN doesn't implement hardware filters, assume all pass for software routing
    return 0;
}

static int virt_can_get_state(can_controller_t* ctrl, can_controller_state_t* out_state) {
    virt_can_priv_t* priv = (virt_can_priv_t*)ctrl->priv;
    if (out_state) *out_state = priv->state;
    return 0;
}

static int virt_can_get_stats(can_controller_t* ctrl, can_controller_stats_t* out_stats) {
    virt_can_priv_t* priv = (virt_can_priv_t*)ctrl->priv;
    if (out_stats) *out_stats = priv->stats;
    return 0;
}

static int virt_can_recover_bus(can_controller_t* ctrl) {
    virt_can_priv_t* priv = (virt_can_priv_t*)ctrl->priv;
    if (priv->state == CAN_CTRL_BUS_OFF) {
        priv->state = CAN_CTRL_RECOVERING;
        // Simulate immediate recovery
        priv->state = CAN_CTRL_ERROR_ACTIVE;
        priv->stats.bus_off_count++;
        return 0;
    }
    return -1;
}

static const can_controller_ops_t virt_can_ops = {
    .start = virt_can_start,
    .stop = virt_can_stop,
    .transmit = virt_can_transmit,
    .set_filters = virt_can_set_filters,
    .get_state = virt_can_get_state,
    .get_stats = virt_can_get_stats,
    .recover_bus = virt_can_recover_bus
};

int virt_can_register(void) {
    g_virt_can_ctrl.name = "virt_can0";
    g_virt_can_ctrl.controller_id = 0;
    g_virt_can_ctrl.ops = &virt_can_ops;
    g_virt_can_ctrl.priv = &g_virt_can_priv;

    memset(&g_virt_can_priv, 0, sizeof(g_virt_can_priv));
    g_virt_can_priv.state = CAN_CTRL_STOPPED;
    g_virt_can_priv.loopback_enabled = true; // default loopback for testing

    return can_controller_core_register(&g_virt_can_ctrl);
}

// Function to poll loopback queue for tests
int virt_can_poll_rx(can_controller_t* ctrl, can_frame_t* out_frame) {
    virt_can_priv_t* priv = (virt_can_priv_t*)ctrl->priv;
    if (priv->count == 0) {
        return -1;
    }
    if (out_frame) {
        *out_frame = priv->rx_queue[priv->head];
    }
    priv->head = (priv->head + 1) % VIRT_CAN_QUEUE_SIZE;
    priv->count--;
    priv->stats.rx_frames++;
    return 0;
}

// Helper to manually set bus off for tests
void virt_can_force_bus_off(can_controller_t* ctrl) {
    virt_can_priv_t* priv = (virt_can_priv_t*)ctrl->priv;
    priv->state = CAN_CTRL_BUS_OFF;
}
