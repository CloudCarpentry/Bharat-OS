#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "stack/can/can_frame.h"
#include "stack/can/can_filter.h"
#include "drivers/can/can_controller.h"

// Message definitions for CAN service IPC

#define CAN_IPC_OP_REGISTER_CLIENT      1
#define CAN_IPC_OP_SUBSCRIBE_FILTER     2
#define CAN_IPC_OP_UNSUBSCRIBE_FILTER   3
#define CAN_IPC_OP_TRANSMIT_FRAME       4
#define CAN_IPC_OP_QUERY_STATS          5
#define CAN_IPC_OP_QUERY_STATE          6
#define CAN_IPC_OP_SET_LOOPBACK         7
#define CAN_IPC_OP_INSTALL_GATEWAY_RULE 8

typedef enum {
    CAN_TX_PRIORITY_BEST_EFFORT = 0,
    CAN_TX_PRIORITY_REALTIME,
    CAN_TX_PRIORITY_CRITICAL
} can_tx_priority_t;

typedef struct {
    uint32_t op;
    uint32_t client_id;
    union {
        struct {
            can_filter_t filter;
        } subscribe;
        struct {
            can_filter_t filter;
        } unsubscribe;
        struct {
            can_frame_t frame;
            can_tx_priority_t priority;
        } transmit;
        struct {
            uint8_t controller_id;
        } query;
        struct {
            uint8_t controller_id;
            bool enable;
        } loopback;
        struct {
            uint8_t src_controller_id;
            uint8_t dst_controller_id;
            can_filter_t filter;
        } gateway;
    };
} can_ipc_request_t;

typedef struct {
    int32_t status;
    union {
        struct {
            uint32_t client_id;
        } register_res;
        struct {
            can_controller_stats_t stats;
        } stats;
        struct {
            can_controller_state_t state;
        } state;
    };
} can_ipc_response_t;
