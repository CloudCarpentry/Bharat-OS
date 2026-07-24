#include "ipc/ipc_profile_policy.h"

static uint16_t urpc_max_payload_for_mem_model(mem_model_t model) {
    switch (model) {
        case MEM_MODEL_MPU:
            return 32U;
        case MEM_MODEL_MMU_LITE:
            return 64U;
        case MEM_MODEL_MMU_FULL:
            return 256U;
        default:
            return 32U;
    }
}

ipc_profile_policy_t ipc_profile_policy_current(void) {
    const mem_model_t model = mem_model_get_current();
    const KernelExecutionProfile exec = get_kernel_execution_profile();

    ipc_profile_policy_t p = {
        .endpoint_payload_max = (uint16_t)BHARAT_IPC_ENDPOINT_PAYLOAD_MAX,
        .max_endpoints = (uint16_t)BHARAT_IPC_MAX_ENDPOINTS,
        .urpc_ring_size = (uint16_t)URPC_RING_SIZE,
        .max_cross_core_payload = urpc_max_payload_for_mem_model(model),
        .prefer_urpc_for_control = (exec != PROFILE_KERNEL_GP),
        .allow_bulk_ipc = (exec != PROFILE_KERNEL_RT),
    };

#if defined(Profile_RTOS)
    p.prefer_urpc_for_control = true;
    p.allow_bulk_ipc = false;
#endif

    if (!mem_model_has_cap(MEM_CAP_TLB_INVALIDATE)) {
        p.max_cross_core_payload = 32U;
    }

    return p;
}

ipc_transport_t ipc_profile_select_transport(ipc_traffic_type_t traffic, bool cross_core) {
    ipc_profile_policy_t p = ipc_profile_policy_current();

    if (!cross_core) {
        return IPC_TRANSPORT_ENDPOINT;
    }

    switch (traffic) {
        case IPC_TRAFFIC_CONTROL:
        case IPC_TRAFFIC_EVENT:
            return p.prefer_urpc_for_control ? IPC_TRANSPORT_URPC : IPC_TRANSPORT_ENDPOINT;
        case IPC_TRAFFIC_BULK:
            return p.allow_bulk_ipc ? IPC_TRANSPORT_ENDPOINT : IPC_TRANSPORT_URPC;
        case IPC_TRAFFIC_TELEMETRY:
        case IPC_TRAFFIC_SERVICE:
        case IPC_TRAFFIC_DEFERRED:
        case IPC_TRAFFIC_UNSPECIFIED:
        default:
            return IPC_TRANSPORT_ENDPOINT;
    }
}

bool ipc_profile_payload_supported(ipc_traffic_type_t traffic,
                                   uint32_t payload_len,
                                   bool cross_core) {
    ipc_profile_policy_t p = ipc_profile_policy_current();

    if (payload_len == 0U) {
        return false;
    }

    if (cross_core && ipc_profile_select_transport(traffic, true) == IPC_TRANSPORT_URPC) {
        return payload_len <= p.max_cross_core_payload;
    }

    return payload_len <= p.endpoint_payload_max;
}
