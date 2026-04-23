#include <assert.h>
#include <stdio.h>

#include "drivers/can/can_controller.h"

static int test_start(can_controller_t* ctrl) {
    return can_controller_set_state(ctrl, CAN_CTRL_ERROR_ACTIVE);
}

static int test_stop(can_controller_t* ctrl) {
    return can_controller_set_state(ctrl, CAN_CTRL_STOPPED);
}

static int test_tx(can_controller_t* ctrl, const can_frame_t* frame) {
    (void)ctrl;
    (void)frame;
    return 0;
}

static int test_set_filters(can_controller_t* ctrl, const can_filter_t* filters, size_t count) {
    (void)ctrl;
    (void)filters;
    (void)count;
    return 0;
}

static int test_set_bitrate(can_controller_t* ctrl, uint32_t nominal, uint32_t data) {
    (void)ctrl;
    return nominal > 0 && data > 0 ? 0 : -1;
}

static int test_set_mode(can_controller_t* ctrl, bool enabled) {
    (void)ctrl;
    (void)enabled;
    return 0;
}

static int test_get_state(can_controller_t* ctrl, can_controller_state_t* out_state) {
    *out_state = ctrl->state;
    return 0;
}

static int test_get_stats(can_controller_t* ctrl, can_controller_stats_t* out_stats) {
    *out_stats = ctrl->stats;
    return 0;
}

static int test_recover(can_controller_t* ctrl) {
    return can_controller_set_state(ctrl, CAN_CTRL_ERROR_ACTIVE);
}

int main(void) {
    can_controller_t ctrl = {
        .name = "testcan0",
        .controller_id = 2,
        .ops = &(can_controller_ops_t){
            .start = test_start,
            .stop = test_stop,
            .transmit = test_tx,
            .set_filters = test_set_filters,
            .set_bitrate = test_set_bitrate,
            .set_loopback = test_set_mode,
            .set_listen_only = test_set_mode,
            .get_state = test_get_state,
            .get_stats = test_get_stats,
            .recover_bus = test_recover,
        },
        .caps = {
            .classical_can = true,
            .can_fd = true,
            .loopback = true,
            .listen_only = true,
            .min_bitrate = 10000,
            .max_bitrate = 1000000,
            .min_data_bitrate = 10000,
            .max_data_bitrate = 2000000,
        },
    };
    can_filter_t filter = {.id = 0x100, .mask = 0x700, .match_extended = false, .extended_only = false};
    can_frame_t frame = {
        .id = 0x123,
        .dlc = 8,
        .data_len = 8,
        .controller_id = ctrl.controller_id,
        .kind = CAN_FRAME_DATA,
    };
    can_frame_t out;

    assert(can_controller_core_register(&ctrl) == 0);
    assert(can_controller_set_state(&ctrl, CAN_CTRL_ERROR_ACTIVE) == 0);
    assert(can_controller_set_bitrate(&ctrl, 500000, 1000000) == 0);
    assert(can_controller_set_filters(&ctrl, &filter, 1) == 0);

    assert(can_controller_enqueue_tx(&ctrl, &frame) == 0);
    assert(can_controller_push_rx(&ctrl, &frame) == 0);
    assert(can_controller_dequeue_rx(&ctrl, &out) == 0);
    assert(out.id == frame.id);

    assert(can_controller_report_error(&ctrl, CAN_CTRL_BUS_OFF) == 0);
    assert(ctrl.state == CAN_CTRL_BUS_OFF);
    assert(can_controller_set_state(&ctrl, CAN_CTRL_RECOVERING) == 0);
    assert(can_controller_set_state(&ctrl, CAN_CTRL_ERROR_ACTIVE) == 0);

    assert(can_controller_set_loopback(&ctrl, true) == 0);
    assert(can_controller_set_listen_only(&ctrl, true) == 0);

    assert(can_controller_core_unregister(&ctrl) == 0);
    printf("test_can_controller_state_machine passed.\n");
    return 0;
}
