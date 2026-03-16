#include "bharat/urpc.h"
#include "hal/hal.h"

// Define channel states per core
static urpc_channel_state_t g_urpc_states[BHARAT_MAX_CPUS];

// Send a command to a target core to initiate binding
int urpc_channel_bind(uint32_t target_core) {
    if (target_core >= BHARAT_MAX_CPUS) return -1;
    if (target_core == hal_cpu_get_id()) return -2; // Can't bind to self

    // Pack a BIND_REQ message
    uint64_t msg = urpc_pack_msg(URPC_BIND_REQ, 0);

    // Update state to binding
    g_urpc_states[target_core] = URPC_CHANNEL_BINDING;

    // Use bootstrap send to transmit the request
    int ret = urpc_bootstrap_send(target_core, msg);
    if (ret != 0) {
        g_urpc_states[target_core] = URPC_CHANNEL_ERROR;
        return ret;
    }

    return 0;
}

// Accept a binding request from a source core
int urpc_channel_accept(uint32_t source_core) {
    if (source_core >= BHARAT_MAX_CPUS) return -1;
    if (source_core == hal_cpu_get_id()) return -2; // Can't accept from self

    // Pack a BIND_ACK message
    uint64_t msg = urpc_pack_msg(URPC_BIND_ACK, 0);

    // We consider ourselves bound once we send the ACK
    g_urpc_states[source_core] = URPC_CHANNEL_BOUND;
    urpc_mark_ready(source_core); // Signal bootstrap layer that channel is ready

    return urpc_bootstrap_send(source_core, msg);
}

// Close a bound channel
int urpc_channel_close(uint32_t target_core) {
    if (target_core >= BHARAT_MAX_CPUS) return -1;

    // Pack a CLOSE message
    uint64_t msg = urpc_pack_msg(URPC_CLOSE, 0);

    g_urpc_states[target_core] = URPC_CHANNEL_CLOSED;

    return urpc_bootstrap_send(target_core, msg);
}

// Get the state of a channel
urpc_channel_state_t urpc_channel_get_state(uint32_t target_core) {
    if (target_core >= BHARAT_MAX_CPUS) return URPC_CHANNEL_ERROR;
    return g_urpc_states[target_core];
}
