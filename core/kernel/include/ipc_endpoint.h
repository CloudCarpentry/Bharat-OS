#ifndef BHARAT_IPC_ENDPOINT_H
#define BHARAT_IPC_ENDPOINT_H

#include <stdint.h>

#include "capability.h"
#include "profile/profile.h"
#include <bharat/uapi/ipc/contract.h>
#include <bharat/uapi/ipc/envelope.h>

#if defined(Profile_RTOS) || defined(BHARAT_PROFILE_RTOS)
#define BHARAT_IPC_MAX_ENDPOINTS 64U
#define BHARAT_IPC_ENDPOINTS_PER_CORE 16U
#elif defined(FEATURE_NUMA_AWARE)
#define BHARAT_IPC_MAX_ENDPOINTS 256U
#define BHARAT_IPC_ENDPOINTS_PER_CORE 64U
#elif defined(CONFIG_MEM_MODEL_MPU) || defined(CONFIG_MEM_MODEL_MMU_LITE)
#define BHARAT_IPC_MAX_ENDPOINTS 64U
#define BHARAT_IPC_ENDPOINTS_PER_CORE 32U
#else
#define BHARAT_IPC_MAX_ENDPOINTS 128U
#define BHARAT_IPC_ENDPOINTS_PER_CORE 64U
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

/*
 * Preserve existing numeric state values 0..3 for full compatibility.
 * Transitional/internal lifecycle states are appended at 4..6.
 */
typedef enum {
    IPC_ENDPOINT_STATE_FREE = 0,
    IPC_ENDPOINT_STATE_READY = 1,
    IPC_ENDPOINT_STATE_CLOSED = 2,
    IPC_ENDPOINT_STATE_REVOKED = 3,
    IPC_ENDPOINT_STATE_ALLOCATING = 4,
    IPC_ENDPOINT_STATE_CLOSING = 5,
    IPC_ENDPOINT_STATE_RECLAIMED = 6,
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
    IPC_ERR_CANCELLED = -9,
    IPC_ERR_REVOKED = -10,
} ipc_status_t;

typedef struct {
    uint64_t sender_pid;
    uint64_t sender_process_object_id;
    uint32_t sender_cspace_id;
    uint32_t sender_owner_core;
    uint64_t delivery_generation;
    cap_type_t cap_type;
    uint64_t cap_rights;
    uint8_t has_provenance;
    uint64_t boot_session_id;
    uint64_t service_incarnation_id;
    uint32_t reply_cap_to_send;
    uint64_t reply_cap_send_rights;
} ipc_provenance_t;

int ipc_endpoint_create(capability_table_t* table, uint32_t* out_send_cap, uint32_t* out_recv_cap);
int ipc_endpoint_send(capability_table_t* table, uint32_t send_cap, const void* payload, uint32_t payload_len, uint64_t timeout_ticks, uint32_t cap_to_send, uint64_t cap_send_rights);
int ipc_endpoint_send_ex(capability_table_t* table, uint32_t send_cap, const void* payload, uint32_t payload_len, uint64_t timeout_ticks, uint32_t cap_to_send, uint64_t cap_send_rights, const ipc_provenance_t* provenance);
int ipc_endpoint_receive(capability_table_t* table, uint32_t recv_cap, void* out_payload, uint32_t out_payload_capacity, uint32_t* out_received_len, uint64_t timeout_ticks, uint32_t* out_received_cap);
int ipc_endpoint_receive_envelope(capability_table_t* table, uint32_t recv_cap, void* out_payload, uint32_t out_payload_capacity, uint32_t* out_received_len, uint64_t timeout_ticks, bh_ipc_receive_envelope_v1_t* out_envelope);

/* Explicit endpoint capability reference and lifecycle operations */
int ipc_endpoint_acquire_send_cap(uint64_t endpoint_obj_ref);
int ipc_endpoint_acquire_recv_cap(uint64_t endpoint_obj_ref);
int ipc_endpoint_release_send_cap(uint64_t endpoint_obj_ref);
int ipc_endpoint_release_recv_cap(uint64_t endpoint_obj_ref);
int ipc_endpoint_owner_close(capability_table_t *table, uint32_t cap_id);
int ipc_endpoint_cancel_waiter(uint64_t endpoint_obj_ref, bh_thread_t *thread, int cancel_reason);
void ipc_endpoint_cancel_all_for_thread(bh_thread_t *thread, int cancel_reason);
uint32_t ipc_endpoint_active_count_for_core(uint32_t core_id);

#endif // BHARAT_IPC_ENDPOINT_H
