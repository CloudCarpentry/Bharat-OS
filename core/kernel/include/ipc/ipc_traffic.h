#ifndef BHARAT_IPC_TRAFFIC_H
#define BHARAT_IPC_TRAFFIC_H

#include <bharat/uapi/ipc/status.h>
#include "ipc_endpoint.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ipc_route_admission {
    bool accepted;
    bharat_status_t reason;
    uint32_t available_slots;
    uint32_t max_payload;
} ipc_route_admission_t;

/**
 * Validates if a specific message route is admissible under traffic, payload,
 * and backpressure constraints.
 */
bharat_status_t ipc_route_admit(
    ipc_traffic_type_t traffic,
    uint32_t payload_len,
    bool cross_core,
    ipc_route_admission_t *out);

#ifdef __cplusplus
}
#endif

#endif // BHARAT_IPC_TRAFFIC_H
