#ifndef BHARAT_IPC_ENDPOINT_H
#define BHARAT_IPC_ENDPOINT_H

#include <stdint.h>

#include "capability.h"
#include "profile/profile.h"

#if defined(Profile_RTOS)
#define BHARAT_IPC_ENDPOINT_PAYLOAD_MAX 64U
#define BHARAT_IPC_MAX_ENDPOINTS 64U
#elif defined(FEATURE_NUMA_AWARE)
#define BHARAT_IPC_ENDPOINT_PAYLOAD_MAX 256U
#define BHARAT_IPC_MAX_ENDPOINTS 256U
#else
#define BHARAT_IPC_ENDPOINT_PAYLOAD_MAX 128U
#define BHARAT_IPC_MAX_ENDPOINTS 128U
#endif

typedef enum {
    IPC_TRAFFIC_UNSPECIFIED = 0,
    IPC_TRAFFIC_CONTROL,
    IPC_TRAFFIC_SERVICE,
    IPC_TRAFFIC_EVENT,
    IPC_TRAFFIC_TELEMETRY,
    IPC_TRAFFIC_BULK,
    IPC_TRAFFIC_DEFERRED,
} ipc_traffic_type_t;

static inline ipc_traffic_type_t ipc_traffic_default(void) {
    return IPC_TRAFFIC_UNSPECIFIED;
}

const char *ipc_traffic_type_name(ipc_traffic_type_t type);

typedef struct {
    uint32_t msg_len;
    uint8_t payload[BHARAT_IPC_ENDPOINT_PAYLOAD_MAX];

    // Capability transfer fields (0 means none attached)
    uint32_t cap_transfer_id;
    uint64_t cap_transfer_rights;
} ipc_message_t;

typedef enum {
    IPC_ENDPOINT_STATE_FREE = 0,
    IPC_ENDPOINT_STATE_READY = 1,
    IPC_ENDPOINT_STATE_CLOSED = 2,
    IPC_ENDPOINT_STATE_REVOKED = 3,
} ipc_endpoint_state_t;

typedef enum {
    IPC_OK = 0,
    IPC_ERR_INVALID = -1,
    IPC_ERR_NO_SPACE = -2,
    IPC_ERR_WOULD_BLOCK = -3,
    IPC_ERR_PERM = -4,
    IPC_ERR_CAP_TRANSFER_NOT_ALLOWED = -5,
    IPC_ERR_CAP_INSTALL_FAILED = -6,
    IPC_ERR_CLOSED = -7,
    IPC_ERR_TIMEOUT = -8,
} ipc_status_t;

int ipc_endpoint_create(capability_table_t* table, uint32_t* out_send_cap, uint32_t* out_recv_cap);
int ipc_endpoint_send(capability_table_t* table, uint32_t send_cap, const void* payload, uint32_t payload_len, uint64_t timeout_ticks, uint32_t cap_to_send, uint64_t cap_send_rights);
int ipc_endpoint_receive(capability_table_t* table, uint32_t recv_cap, void* out_payload, uint32_t out_payload_capacity, uint32_t* out_received_len, uint64_t timeout_ticks, uint32_t* out_received_cap);

#endif // BHARAT_IPC_ENDPOINT_H
