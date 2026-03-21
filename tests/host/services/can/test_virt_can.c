#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include "bharat/drivers/can_controller.h"
// This is a host unit test for the virtual CAN controller and common service logic
// Therefore, we compile it with standard headers

// Mock or declare what we need
void virt_can_register(void);
can_controller_t* get_virt_can_ctrl(void);

// Basic Assert macro
#define ASSERT_TEST(cond, msg) \
    do { \
        if (!(cond)) { \
            fprintf(stderr, "[FAILED] %s\n", msg); \
            exit(1); \
        } \
    } while(0)

static void test_virt_can_lifecycle(void) {
    printf("Running test_virt_can_lifecycle...\n");
    virt_can_register();
    can_controller_t* ctrl = get_virt_can_ctrl();

    ASSERT_TEST(ctrl != NULL, "Controller should not be NULL");
    ASSERT_TEST(ctrl->ops != NULL, "Ops vtable should not be NULL");
    ASSERT_TEST(ctrl->state == CAN_STATE_STOPPED, "Initial state should be STOPPED");

    ctrl->ops->start(ctrl);
    ASSERT_TEST(ctrl->state == CAN_STATE_ERROR_ACTIVE, "State should be ERROR_ACTIVE after start");

    ctrl->ops->stop(ctrl);
    ASSERT_TEST(ctrl->state == CAN_STATE_STOPPED, "State should be STOPPED after stop");

    printf("test_virt_can_lifecycle passed.\n");
}

static void test_virt_can_loopback(void) {
    printf("Running test_virt_can_loopback...\n");
    virt_can_register();
    can_controller_t* ctrl = get_virt_can_ctrl();

    ctrl->ops->start(ctrl);

    // Enable loopback
    ctrl->ops->set_loopback(ctrl, true);

    can_frame_t send_frame;
    memset(&send_frame, 0, sizeof(send_frame));
    send_frame.id = 0x123;
    send_frame.id_type = CAN_ID_TYPE_STANDARD;
    send_frame.dlc = 8;
    send_frame.is_fd = false;
    for (int i=0; i<8; i++) send_frame.payload[i] = i;

    int err = ctrl->ops->send(ctrl, &send_frame);
    ASSERT_TEST(err == 0, "Send should succeed in loopback mode");

    can_frame_t recv_frame;
    memset(&recv_frame, 0, sizeof(recv_frame));
    err = ctrl->ops->recv(ctrl, &recv_frame);

    ASSERT_TEST(err == 0, "Recv should succeed");
    ASSERT_TEST(recv_frame.id == 0x123, "Received ID mismatch");
    ASSERT_TEST(recv_frame.dlc == 8, "Received DLC mismatch");
    ASSERT_TEST(recv_frame.payload[0] == 0, "Received payload mismatch");

    can_controller_stats_t stats;
    ctrl->ops->get_stats(ctrl, &stats);
    ASSERT_TEST(stats.tx_frames == 1, "Tx frames stat should be 1");
    ASSERT_TEST(stats.rx_frames == 1, "Rx frames stat should be 1");

    ctrl->ops->stop(ctrl);
    printf("test_virt_can_loopback passed.\n");
}

static void test_virt_can_fd_frame(void) {
    printf("Running test_virt_can_fd_frame...\n");
    virt_can_register();
    can_controller_t* ctrl = get_virt_can_ctrl();

    ctrl->ops->start(ctrl);
    ctrl->ops->set_loopback(ctrl, true);

    can_frame_t send_frame;
    memset(&send_frame, 0, sizeof(send_frame));
    send_frame.id = 0x1F000000;
    send_frame.id_type = CAN_ID_TYPE_EXTENDED;
    send_frame.dlc = 64; // Max FD payload length
    send_frame.is_fd = true;
    send_frame.brs = true; // Bit Rate Switch on
    for (int i=0; i<64; i++) send_frame.payload[i] = (uint8_t)i;

    int err = ctrl->ops->send(ctrl, &send_frame);
    ASSERT_TEST(err == 0, "FD Send should succeed in loopback mode");

    can_frame_t recv_frame;
    memset(&recv_frame, 0, sizeof(recv_frame));
    err = ctrl->ops->recv(ctrl, &recv_frame);

    ASSERT_TEST(err == 0, "FD Recv should succeed");
    ASSERT_TEST(recv_frame.id == 0x1F000000, "FD Received ID mismatch");
    ASSERT_TEST(recv_frame.id_type == CAN_ID_TYPE_EXTENDED, "FD ID Type mismatch");
    ASSERT_TEST(recv_frame.dlc == 64, "FD Received DLC mismatch");
    ASSERT_TEST(recv_frame.is_fd == true, "FD flag mismatch");
    ASSERT_TEST(recv_frame.brs == true, "FD BRS flag mismatch");
    ASSERT_TEST(recv_frame.payload[63] == 63, "FD Received payload mismatch");

    ctrl->ops->stop(ctrl);
    printf("test_virt_can_fd_frame passed.\n");
}

int main(void) {
    printf("Starting CAN subsystem host tests...\n");
    test_virt_can_lifecycle();
    test_virt_can_loopback();
    test_virt_can_fd_frame();
    printf("All CAN subsystem host tests passed!\n");
    return 0;
}
