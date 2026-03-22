#include "ipc_endpoint.h"

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
