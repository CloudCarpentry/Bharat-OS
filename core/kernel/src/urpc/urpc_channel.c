#include "bharat/urpc.h"
#include "hal/hal.h"

// Define channel states per core
static urpc_channel_state_t g_urpc_states[BHARAT_MAX_CPUS];

// Validate state transitions
bool urpc_channel_transition_allowed(urpc_channel_state_t from, urpc_channel_state_t to) {
    if (from == to) return true;

    switch (from) {
        case URPC_CHANNEL_CLOSED:
            return (to == URPC_CHANNEL_BINDING) || (to == URPC_CHANNEL_BOUND);
        case URPC_CHANNEL_BINDING:
            return (to == URPC_CHANNEL_BOUND) || (to == URPC_CHANNEL_ERROR) || (to == URPC_CHANNEL_CLOSED);
        case URPC_CHANNEL_BOUND:
            return (to == URPC_CHANNEL_CLOSED) || (to == URPC_CHANNEL_ERROR);
        case URPC_CHANNEL_ERROR:
            return (to == URPC_CHANNEL_CLOSED); // Must reset to closed before retry
        default:
            return false;
    }
}

// Send a command to a target core to initiate binding
bharat_status_t urpc_channel_bind(uint32_t target_core) {
    if (target_core >= BHARAT_MAX_CPUS) return BHARAT_IPC_STATUS_ERR_NOT_FOUND;
    if (target_core == hal_cpu_get_id()) return BHARAT_IPC_STATUS_ERR_UNSUPPORTED; // Can't bind to self

    urpc_channel_state_t current = g_urpc_states[target_core];
    if (current == URPC_CHANNEL_BINDING || current == URPC_CHANNEL_BOUND) {
        return BHARAT_IPC_STATUS_ERR_INTERNAL; // Already in progress or bound (Busy)
    }

    if (!urpc_channel_transition_allowed(current, URPC_CHANNEL_BINDING)) {
        return BHARAT_IPC_STATUS_ERR_INTERNAL;
    }

    // Pack a BIND_REQ message
    uint64_t msg = urpc_pack_msg(URPC_BIND_REQ, 0);

    // Update state to binding
    g_urpc_states[target_core] = URPC_CHANNEL_BINDING;

    // Use bootstrap send to transmit the request
    int ret = urpc_bootstrap_send(target_core, msg);
    if (ret != 0) {
        g_urpc_states[target_core] = URPC_CHANNEL_ERROR;
        return BHARAT_IPC_STATUS_ERR_INTERNAL;
    }

    return BHARAT_IPC_STATUS_OK;
}

// Accept a binding request from a source core
bharat_status_t urpc_channel_accept(uint32_t source_core) {
    if (source_core >= BHARAT_MAX_CPUS) return BHARAT_IPC_STATUS_ERR_NOT_FOUND;
    if (source_core == hal_cpu_get_id()) return BHARAT_IPC_STATUS_ERR_UNSUPPORTED; // Can't accept from self

    urpc_channel_state_t current = g_urpc_states[source_core];
    if (!urpc_channel_transition_allowed(current, URPC_CHANNEL_BOUND)) {
        return BHARAT_IPC_STATUS_ERR_INTERNAL;
    }

    // Pack a BIND_ACK message
    uint64_t msg = urpc_pack_msg(URPC_BIND_ACK, 0);

    // We consider ourselves bound once we send the ACK
    g_urpc_states[source_core] = URPC_CHANNEL_BOUND;
    urpc_mark_ready(source_core); // Signal bootstrap layer that channel is ready

    int ret = urpc_bootstrap_send(source_core, msg);
    if (ret != 0) {
        g_urpc_states[source_core] = URPC_CHANNEL_ERROR;
        return BHARAT_IPC_STATUS_ERR_INTERNAL;
    }

    return BHARAT_IPC_STATUS_OK;
}

// Close a bound channel
bharat_status_t urpc_channel_close(uint32_t target_core) {
    if (target_core >= BHARAT_MAX_CPUS) return BHARAT_IPC_STATUS_ERR_NOT_FOUND;

    urpc_channel_state_t current = g_urpc_states[target_core];
    if (!urpc_channel_transition_allowed(current, URPC_CHANNEL_CLOSED)) {
        return BHARAT_IPC_STATUS_ERR_INTERNAL;
    }

    // Pack a CLOSE message
    uint64_t msg = urpc_pack_msg(URPC_CLOSE, 0);

    g_urpc_states[target_core] = URPC_CHANNEL_CLOSED;

    int ret = urpc_bootstrap_send(target_core, msg);
    if (ret != 0) {
         // Even if send fails, we are logically closed locally
         return BHARAT_IPC_STATUS_ERR_INTERNAL;
    }

    return BHARAT_IPC_STATUS_OK;
}

// Get the state of a channel
urpc_channel_state_t urpc_channel_get_state(uint32_t target_core) {
    if (target_core >= BHARAT_MAX_CPUS) return URPC_CHANNEL_ERROR;
    return g_urpc_states[target_core];
}

int urpc_channel_can_route(ipc_traffic_type_t traffic, uint32_t payload_len, bool cross_core) {
    return ipc_profile_select_transport(traffic, cross_core) == IPC_TRANSPORT_URPC &&
           ipc_profile_payload_supported(traffic, payload_len, cross_core);
}
