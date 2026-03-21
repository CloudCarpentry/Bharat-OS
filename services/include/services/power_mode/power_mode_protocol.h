#pragma once
#include "services/power_mode/power_mode.h"

// Define IPC messages for power mode manager (for other services to communicate)

#define PM_IPC_OP_REGISTER        1
#define PM_IPC_OP_REQUEST         2
#define PM_IPC_OP_GET_CURRENT     3
#define PM_IPC_OP_FORCE_LIMP_HOME 4

typedef struct {
    uint32_t op;
    union {
        struct {
            power_mode_state_t target;
            power_mode_reason_t reason;
        } request;
        struct {
            power_mode_reason_t reason;
        } limp_home;
    };
} power_mode_ipc_request_t;

typedef struct {
    int32_t status;
    union {
        struct {
            power_mode_state_t state;
        } current;
    };
} power_mode_ipc_response_t;
