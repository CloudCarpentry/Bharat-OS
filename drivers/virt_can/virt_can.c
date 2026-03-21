#include "bharat/drivers/can_controller.h"
#include <string.h>

#define VIRT_CAN_QUEUE_SIZE 128

typedef struct {
    can_frame_t rx_queue[VIRT_CAN_QUEUE_SIZE];
    uint32_t head;
    uint32_t tail;
    uint32_t count;
    bool loopback_enabled;
    bool listen_only_enabled;
} virt_can_priv_t;

static virt_can_priv_t g_virt_can_priv;
static can_controller_t g_virt_can_ctrl;

static int virt_can_init(can_controller_t* ctrl) {
    virt_can_priv_t* priv = (virt_can_priv_t*)ctrl->priv_data;
    memset(priv, 0, sizeof(virt_can_priv_t));
    ctrl->state = CAN_STATE_STOPPED;
    memset(&ctrl->stats, 0, sizeof(can_controller_stats_t));
    return 0;
}

static int virt_can_start(can_controller_t* ctrl) {
    if (ctrl->state != CAN_STATE_STOPPED) {
        return -1; // Already started
    }
    ctrl->state = CAN_STATE_ERROR_ACTIVE;
    return 0;
}

static int virt_can_stop(can_controller_t* ctrl) {
    ctrl->state = CAN_STATE_STOPPED;
    return 0;
}

static int virt_can_send(can_controller_t* ctrl, const can_frame_t* frame) {
    if (ctrl->state == CAN_STATE_STOPPED || ctrl->state == CAN_STATE_BUS_OFF) {
        return -1;
    }

    virt_can_priv_t* priv = (virt_can_priv_t*)ctrl->priv_data;

    if (priv->listen_only_enabled) {
        return -1; // Tx not allowed in listen-only
    }

    ctrl->stats.tx_frames++;

    // In virt_can, sending a frame loops it back if loopback is enabled
    if (priv->loopback_enabled) {
        if (priv->count >= VIRT_CAN_QUEUE_SIZE) {
            ctrl->stats.tx_errors++;
            return -1; // Tx Queue full
        }

        priv->rx_queue[priv->tail] = *frame;
        priv->tail = (priv->tail + 1) % VIRT_CAN_QUEUE_SIZE;
        priv->count++;
    }

    return 0;
}

static int virt_can_recv(can_controller_t* ctrl, can_frame_t* frame) {
    virt_can_priv_t* priv = (virt_can_priv_t*)ctrl->priv_data;

    if (priv->count == 0) {
        return -1; // Queue empty
    }

    *frame = priv->rx_queue[priv->head];
    priv->head = (priv->head + 1) % VIRT_CAN_QUEUE_SIZE;
    priv->count--;

    ctrl->stats.rx_frames++;

    return 0;
}

static int virt_can_add_filter(can_controller_t* ctrl, const can_hardware_filter_t* filter, uint32_t* filter_id) {
    // Virt CAN doesn't implement hardware filters, assume all pass for software routing
    (void)ctrl; (void)filter;
    if (filter_id) *filter_id = 0;
    return 0;
}

static int virt_can_remove_filter(can_controller_t* ctrl, uint32_t filter_id) {
    (void)ctrl; (void)filter_id;
    return 0;
}

static int virt_can_set_loopback(can_controller_t* ctrl, bool enable) {
    virt_can_priv_t* priv = (virt_can_priv_t*)ctrl->priv_data;
    priv->loopback_enabled = enable;
    return 0;
}

static int virt_can_set_listen_only(can_controller_t* ctrl, bool enable) {
    virt_can_priv_t* priv = (virt_can_priv_t*)ctrl->priv_data;
    priv->listen_only_enabled = enable;
    return 0;
}

static int virt_can_recover_bus_off(can_controller_t* ctrl) {
    if (ctrl->state == CAN_STATE_BUS_OFF) {
        ctrl->state = CAN_STATE_RECOVERING;
        // Simulate immediate recovery
        ctrl->state = CAN_STATE_ERROR_ACTIVE;
        ctrl->stats.bus_off_count++;
        return 0;
    }
    return -1;
}

static can_controller_state_t virt_can_get_state(can_controller_t* ctrl) {
    return ctrl->state;
}

static int virt_can_get_stats(can_controller_t* ctrl, can_controller_stats_t* stats) {
    if (!stats) return -1;
    *stats = ctrl->stats;
    return 0;
}

static const can_controller_ops_t virt_can_ops = {
    .init = virt_can_init,
    .start = virt_can_start,
    .stop = virt_can_stop,
    .send = virt_can_send,
    .recv = virt_can_recv,
    .add_filter = virt_can_add_filter,
    .remove_filter = virt_can_remove_filter,
    .set_loopback = virt_can_set_loopback,
    .set_listen_only = virt_can_set_listen_only,
    .recover_bus_off = virt_can_recover_bus_off,
    .get_state = virt_can_get_state,
    .get_stats = virt_can_get_stats
};

void virt_can_register(void) {
    g_virt_can_ctrl.name = "virt_can0";
    g_virt_can_ctrl.ops = &virt_can_ops;
    g_virt_can_ctrl.priv_data = &g_virt_can_priv;

    // In a full implementation, we would call can_controller_core_register(&g_virt_can_ctrl) here
    // For now, we manually init to avoid undefined references.
    g_virt_can_ctrl.ops->init(&g_virt_can_ctrl);
}

can_controller_t* get_virt_can_ctrl(void) {
    return &g_virt_can_ctrl;
}
