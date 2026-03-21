#pragma once
#include "services/diag/diag_event.h"

// IPC Message Definitions for Diagnostics

#define DIAG_IPC_OP_REPORT 1
#define DIAG_IPC_OP_CLEAR  2
#define DIAG_IPC_OP_QUERY  3

typedef struct {
    uint32_t op;
    union {
        struct {
            diag_event_t event;
        } report;
        struct {
            uint32_t dtc;
        } clear;
        struct {
            uint32_t dtc;
        } query;
    };
} diag_ipc_request_t;

typedef struct {
    int32_t status;
    union {
        struct {
            diag_event_t event;
        } query;
    };
} diag_ipc_response_t;
