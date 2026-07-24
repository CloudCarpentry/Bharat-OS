#include "ipc_endpoint.h"
#include "ipc/ipc_traffic.h"
#include "ipc/ipc_profile_policy.h"
#include <stddef.h>

static const char *k_ipc_traffic_names[] = {
    [IPC_TRAFFIC_UNSPECIFIED] = "UNSPECIFIED",
    [IPC_TRAFFIC_CONTROL]     = "CONTROL",
    [IPC_TRAFFIC_SERVICE]     = "SERVICE",
    [IPC_TRAFFIC_EVENT]       = "EVENT",
    [IPC_TRAFFIC_TELEMETRY]   = "TELEMETRY",
    [IPC_TRAFFIC_BULK]        = "BULK",
    [IPC_TRAFFIC_DEFERRED]    = "DEFERRED",
};

const char *ipc_traffic_type_name(ipc_traffic_type_t type) {
    if (type >= 0 && type <= IPC_TRAFFIC_DEFERRED) {
        return k_ipc_traffic_names[type];
    }
    return "UNKNOWN";
}

bharat_status_t ipc_route_admit(
    ipc_traffic_type_t traffic,
    uint32_t payload_len,
    bool cross_core,
    ipc_route_admission_t *out)
{
    if (out == NULL) {
        return BHARAT_IPC_STATUS_ERR_INTERNAL;
    }

    out->accepted = false;
    out->available_slots = 0; // Placeholder until full backpressure implemented

    ipc_profile_policy_t p = ipc_profile_policy_current();
    ipc_transport_t transport = ipc_profile_select_transport(traffic, cross_core);

    if (cross_core) {
        out->max_payload = (transport == IPC_TRANSPORT_URPC) ?
                            p.max_cross_core_payload : p.endpoint_payload_max;
    } else {
        out->max_payload = p.endpoint_payload_max;
    }

    if (!ipc_profile_payload_supported(traffic, payload_len, cross_core)) {
        out->reason = BHARAT_IPC_STATUS_ERR_LENGTH;
        return BHARAT_IPC_STATUS_ERR_LENGTH;
    }

    // Baseline backpressure: for now, we always admit if payload is valid.
    // In IPC1, this will check actual ring/buffer availability.
    out->accepted = true;
    out->reason = BHARAT_IPC_STATUS_OK;
    out->available_slots = 1; // At least one slot available if accepted

    return BHARAT_IPC_STATUS_OK;
}
