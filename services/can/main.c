#include "can_service.h"
#include <stdio.h>
#include <string.h>
// Since this is a user-space service stub, we'd normally include IPC libs:
// #include "bharat/ipc.h"
// #include "bharat/cap.h"
// And our new driver interface, mapped for user-space access or wrapped by system calls.
// For now, we mock the basic structure to show the architecture.

#include "bharat/drivers/can_controller.h"

void virt_can_register(void);
can_controller_t* get_virt_can_ctrl(void);

typedef struct {
    capability_id_t client_cap;
    bool active;
    uint32_t filters[CAN_SVC_MAX_FILTERS];
    uint8_t filter_count;
} can_client_session_t;

static can_client_session_t g_sessions[CAN_SVC_MAX_CLIENTS];
static can_controller_t* g_ctrl;

static void can_policy_init(void) {
    memset(g_sessions, 0, sizeof(g_sessions));
}

static bool can_policy_check_tx(capability_id_t cap, uint32_t can_id) {
    // In a real system, verify the cap table allows this client to send 'can_id'
    (void)cap; (void)can_id;
    return true;
}

static bool can_policy_check_rx(capability_id_t cap, uint32_t can_id) {
    // In a real system, verify the cap table allows this client to recv 'can_id'
    (void)cap; (void)can_id;
    return true;
}

void can_service_init(void) {
    printf("[CAN_SVC] Initializing CAN routing service...\n");
    can_policy_init();

    // Register the virtual CAN controller for testing
    virt_can_register();
    g_ctrl = get_virt_can_ctrl();

    if (g_ctrl && g_ctrl->ops->start) {
        g_ctrl->ops->start(g_ctrl);
        printf("[CAN_SVC] Controller started: %s\n", g_ctrl->name);
    }
}

static void handle_ipc_send(capability_id_t client, const can_frame_t* frame) {
    if (!can_policy_check_tx(client, frame->id)) {
        printf("[CAN_SVC] TX capability check failed for client %u, ID: 0x%x\n", client, frame->id);
        return;
    }

    if (g_ctrl && g_ctrl->ops->send) {
        int err = g_ctrl->ops->send(g_ctrl, frame);
        if (err != 0) {
            printf("[CAN_SVC] Frame Tx error: %d\n", err);
        }
    }
}

static void handle_ipc_recv_poll(capability_id_t client, can_frame_t* out_frame) {
    if (g_ctrl && g_ctrl->ops->recv) {
        int err = g_ctrl->ops->recv(g_ctrl, out_frame);
        if (err == 0) {
            // Valid frame received. Check if client is allowed to see it.
            if (!can_policy_check_rx(client, out_frame->id)) {
                // Drop or re-queue depending on design. Here we zero it out.
                memset(out_frame, 0, sizeof(*out_frame));
            }
        }
    }
}

void can_service_loop(void) {
    printf("[CAN_SVC] Entering main IPC event loop...\n");
    // Pseudo-code for IPC loop:
    // while (1) {
    //     bharat_ipc_msg_t msg;
    //     bharat_ipc_recv(CAN_SVC_ENDPOINT_ID, &msg);
    //
    //     switch (msg.type) {
    //         case CAN_IPC_MSG_SEND_FRAME:
    //             handle_ipc_send(msg.sender_cap, (can_frame_t*)msg.payload);
    //             break;
    //         case CAN_IPC_MSG_RECV_FRAME:
    //             handle_ipc_recv_poll(msg.sender_cap, (can_frame_t*)msg.reply_buf);
    //             bharat_ipc_reply(msg.sender_cap);
    //             break;
    //     }
    // }
}
