#ifndef BHARAT_UAPI_SERVICEMGR_HEALTH_H
#define BHARAT_UAPI_SERVICEMGR_HEALTH_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Service Readiness, Liveness, and Health Contract
 *
 * - Readiness: The ability of the service to accept and process traffic or requests.
 * - Liveness: The binary state indicating if the service process is currently running/alive.
 * - Health: The internal semantic well-being of the service's application logic.
 */

typedef enum {
    SM_HEALTH_STATE_DECLARED = 0,
    SM_HEALTH_STATE_SPAWNED = 1,
    SM_HEALTH_STATE_BOUND = 2,
    SM_HEALTH_STATE_READY = 3,
    SM_HEALTH_STATE_DEGRADED = 4,
    SM_HEALTH_STATE_FAILED = 5,
    SM_HEALTH_STATE_STOPPED = 6
} sm_health_state_t;

typedef struct {
    uint32_t struct_size;          // Versioning and validation boundary
    uint32_t service_id;           // Identifies the service
    uint64_t process_id;           // PM identity
    uint64_t boot_session_id;      // Protects against stale messages from prior boots
    uint64_t incarnation_id;       // Protects against stale messages from prior service restarts
    uint32_t health_state;         // sm_health_state_t, explicitly sized to avoid ABI issues
    uint32_t padding;              // Explicit padding to zero out uninitialized memory
    uint64_t timestamp_ticks;
    uint64_t heartbeat_sequence;
    uint64_t deadline_ticks;
    int32_t status_code;
} sm_req_health_update_t;

#define SM_HEALTH_UPDATE_MAX_PAYLOAD_SIZE sizeof(sm_req_health_update_t)

typedef struct {
    int32_t status;
} sm_resp_health_update_t;

#ifdef __cplusplus
}
#endif

#endif // BHARAT_UAPI_SERVICEMGR_HEALTH_H
