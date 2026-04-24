#include "drivers/can/can_controller.h"

typedef struct {
    can_filter_t filters[CAN_CTRL_MAX_FILTERS];
    size_t filter_count;

    can_frame_t tx_queue[CAN_CTRL_MAX_TX_QUEUE];
    uint16_t tx_head;
    uint16_t tx_tail;
    uint16_t tx_count;

    can_frame_t rx_queue[CAN_CTRL_MAX_RX_QUEUE];
    uint16_t rx_head;
    uint16_t rx_tail;
    uint16_t rx_count;
} can_controller_ctx_t;

static can_controller_t* g_can_controllers[CAN_CTRL_MAX_CONTROLLERS];
static can_controller_ctx_t g_can_ctx[CAN_CTRL_MAX_CONTROLLERS];

static can_controller_ctx_t* can_ctx(can_controller_t* ctrl) {
    if (!ctrl || ctrl->controller_id >= CAN_CTRL_MAX_CONTROLLERS) {
        return 0;
    }
    return &g_can_ctx[ctrl->controller_id];
}

int can_controller_validate_frame(const can_frame_t* frame, bool allow_fd) {
    uint8_t max_len;

    if (!frame) {
        return -1;
    }
    if (frame->kind == CAN_FRAME_ERROR) {
        return 0;
    }

    max_len = frame->is_fd ? 64U : 8U;
    if (frame->is_fd && !allow_fd) {
        return -1;
    }
    if (frame->data_len > max_len || frame->dlc > max_len) {
        return -1;
    }
    return 0;
}

int can_controller_core_register(can_controller_t* ctrl) {
    can_controller_ctx_t* ctx;

    if (!ctrl || !ctrl->ops || !ctrl->name) {
        return -1;
    }
    if (ctrl->controller_id >= CAN_CTRL_MAX_CONTROLLERS) {
        return -1;
    }
    if (g_can_controllers[ctrl->controller_id]) {
        return -1;
    }
    if (ctrl->caps.min_bitrate == 0 || ctrl->caps.max_bitrate < ctrl->caps.min_bitrate) {
        return -1;
    }

    ctrl->state = CAN_CTRL_STOPPED;
    ctrl->nominal_bitrate = ctrl->caps.min_bitrate;
    ctrl->data_bitrate = ctrl->caps.can_fd ? ctrl->caps.min_data_bitrate : ctrl->caps.min_bitrate;
    ctrl->loopback_enabled = false;
    ctrl->listen_only_enabled = false;

    ctx = can_ctx(ctrl);
    if (!ctx) {
        return -1;
    }
    ctx->filter_count = 0;
    ctx->tx_head = 0;
    ctx->tx_tail = 0;
    ctx->tx_count = 0;
    ctx->rx_head = 0;
    ctx->rx_tail = 0;
    ctx->rx_count = 0;

    g_can_controllers[ctrl->controller_id] = ctrl;
    return 0;
}

int can_controller_core_unregister(can_controller_t* ctrl) {
    if (!ctrl || ctrl->controller_id >= CAN_CTRL_MAX_CONTROLLERS) {
        return -1;
    }

    if (g_can_controllers[ctrl->controller_id] != ctrl) {
        return -1;
    }

    g_can_controllers[ctrl->controller_id] = 0;
    return 0;
}

can_controller_t* can_controller_core_get(uint8_t controller_id) {
    if (controller_id >= CAN_CTRL_MAX_CONTROLLERS) {
        return 0;
    }
    return g_can_controllers[controller_id];
}

int can_controller_set_state(can_controller_t* ctrl, can_controller_state_t new_state) {
    if (!ctrl) {
        return -1;
    }

    if (new_state == CAN_CTRL_RECOVERING && ctrl->state != CAN_CTRL_BUS_OFF) {
        return -1;
    }
    ctrl->state = new_state;
    if (new_state == CAN_CTRL_BUS_OFF) {
        ctrl->stats.bus_off_count++;
    } else if (new_state == CAN_CTRL_ERROR_PASSIVE) {
        ctrl->stats.error_passive_count++;
    }
    return 0;
}

int can_controller_set_bitrate(can_controller_t* ctrl, uint32_t nominal_bitrate, uint32_t data_bitrate) {
    if (!ctrl || nominal_bitrate < ctrl->caps.min_bitrate || nominal_bitrate > ctrl->caps.max_bitrate) {
        return -1;
    }
    if (ctrl->caps.can_fd) {
        if (data_bitrate < ctrl->caps.min_data_bitrate || data_bitrate > ctrl->caps.max_data_bitrate) {
            return -1;
        }
    } else {
        data_bitrate = nominal_bitrate;
    }

    if (ctrl->ops->set_bitrate && ctrl->ops->set_bitrate(ctrl, nominal_bitrate, data_bitrate) != 0) {
        return -1;
    }

    ctrl->nominal_bitrate = nominal_bitrate;
    ctrl->data_bitrate = data_bitrate;
    return 0;
}

int can_controller_set_filters(can_controller_t* ctrl, const can_filter_t* filters, size_t count) {
    size_t i;
    can_controller_ctx_t* ctx = can_ctx(ctrl);

    if (!ctrl || !ctx || (!filters && count != 0) || count > CAN_CTRL_MAX_FILTERS) {
        return -1;
    }

    for (i = 0; i < count; ++i) {
        ctx->filters[i] = filters[i];
    }
    ctx->filter_count = count;

    if (ctrl->ops->set_filters) {
        return ctrl->ops->set_filters(ctrl, filters, count);
    }
    return 0;
}

int can_controller_enqueue_tx(can_controller_t* ctrl, const can_frame_t* frame) {
    can_controller_ctx_t* ctx = can_ctx(ctrl);

    if (!ctrl || !ctx || can_controller_validate_frame(frame, ctrl->caps.can_fd) != 0) {
        return -1;
    }
    if (ctrl->state != CAN_CTRL_ERROR_ACTIVE && ctrl->state != CAN_CTRL_ERROR_PASSIVE) {
        return -1;
    }
    if (ctx->tx_count >= CAN_CTRL_MAX_TX_QUEUE) {
        ctrl->stats.tx_drops++;
        return -1;
    }

    ctx->tx_queue[ctx->tx_tail] = *frame;
    ctx->tx_tail = (uint16_t)((ctx->tx_tail + 1U) % CAN_CTRL_MAX_TX_QUEUE);
    ctx->tx_count++;

    if (ctrl->ops->transmit && ctrl->ops->transmit(ctrl, frame) != 0) {
        ctrl->stats.tx_drops++;
        return -1;
    }

    ctrl->stats.tx_frames++;
    return 0;
}

int can_controller_push_rx(can_controller_t* ctrl, const can_frame_t* frame) {
    can_controller_ctx_t* ctx = can_ctx(ctrl);
    size_t i;
    bool matched;

    if (!ctrl || !ctx || can_controller_validate_frame(frame, ctrl->caps.can_fd) != 0) {
        return -1;
    }
    if (ctx->rx_count >= CAN_CTRL_MAX_RX_QUEUE) {
        ctrl->stats.rx_overrun_count++;
        ctrl->stats.rx_drops++;
        return -1;
    }

    matched = (ctx->filter_count == 0);
    for (i = 0; i < ctx->filter_count; ++i) {
        if (can_filter_match(&ctx->filters[i], frame)) {
            matched = true;
            break;
        }
    }
    if (!matched) {
        ctrl->stats.rx_drops++;
        return -1;
    }

    ctx->rx_queue[ctx->rx_tail] = *frame;
    ctx->rx_tail = (uint16_t)((ctx->rx_tail + 1U) % CAN_CTRL_MAX_RX_QUEUE);
    ctx->rx_count++;
    return 0;
}

int can_controller_dequeue_rx(can_controller_t* ctrl, can_frame_t* out_frame) {
    can_controller_ctx_t* ctx = can_ctx(ctrl);

    if (!ctrl || !ctx || !out_frame || ctx->rx_count == 0) {
        return -1;
    }

    *out_frame = ctx->rx_queue[ctx->rx_head];
    ctx->rx_head = (uint16_t)((ctx->rx_head + 1U) % CAN_CTRL_MAX_RX_QUEUE);
    ctx->rx_count--;
    ctrl->stats.rx_frames++;
    return 0;
}

int can_controller_set_loopback(can_controller_t* ctrl, bool enabled) {
    if (!ctrl || !ctrl->caps.loopback) {
        return -1;
    }
    if (ctrl->ops->set_loopback && ctrl->ops->set_loopback(ctrl, enabled) != 0) {
        return -1;
    }
    ctrl->loopback_enabled = enabled;
    return 0;
}

int can_controller_set_listen_only(can_controller_t* ctrl, bool enabled) {
    if (!ctrl || !ctrl->caps.listen_only) {
        return -1;
    }
    if (ctrl->ops->set_listen_only && ctrl->ops->set_listen_only(ctrl, enabled) != 0) {
        return -1;
    }
    ctrl->listen_only_enabled = enabled;
    return 0;
}

int can_controller_report_error(can_controller_t* ctrl, can_controller_state_t new_state) {
    if (!ctrl) {
        return -1;
    }
    if (new_state == CAN_CTRL_BUS_OFF) {
        ctrl->stats.error_frames++;
    }
    return can_controller_set_state(ctrl, new_state);
}
