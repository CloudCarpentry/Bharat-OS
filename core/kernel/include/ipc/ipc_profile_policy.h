#ifndef BHARAT_IPC_PROFILE_POLICY_H
#define BHARAT_IPC_PROFILE_POLICY_H

#include <stdbool.h>
#include <stdint.h>

#include "ipc_endpoint.h"
#include "mm/mem_model.h"
#include "profile/profile.h"
#include "urpc/urpc_bootstrap.h"

typedef enum {
    IPC_TRANSPORT_ENDPOINT = 0,
    IPC_TRANSPORT_URPC = 1,
} ipc_transport_t;

typedef struct {
    uint16_t endpoint_payload_max;
    uint16_t max_endpoints;
    uint16_t urpc_ring_size;
    uint16_t max_cross_core_payload;
    bool prefer_urpc_for_control;
    bool allow_bulk_ipc;
} ipc_profile_policy_t;

ipc_profile_policy_t ipc_profile_policy_current(void);
ipc_transport_t ipc_profile_select_transport(ipc_traffic_type_t traffic, bool cross_core);
bool ipc_profile_payload_supported(ipc_traffic_type_t traffic,
                                   uint32_t payload_len,
                                   bool cross_core);

#endif // BHARAT_IPC_PROFILE_POLICY_H
