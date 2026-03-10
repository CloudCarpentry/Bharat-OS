#include "../../include/ipc/mk_dispatch.h"
#include "../../include/hal/hal.h"
#include "../../include/sched.h"

#define MK_MSG_TYPE_AI_SUGGESTION 1U

// Simple first-pass authorization stub
static int mk_authorize_message(mk_channel_t *channel, urpc_msg_t *msg) {
    (void)channel;
    (void)msg;
    // For now, accept generic safe types but reject unrecognized or sensitive types implicitly
    // This will be expanded when full distributed capabilities and tokens are ready.
    if (msg->msg_type == MK_MSG_TYPE_AI_SUGGESTION) {
        return 0; // Temporarily allow AI suggestions until proper policies are wired
    }

    // Default route: allow control messages but this will tighten
    return 0;
}

int mk_dispatch_message(mk_channel_t *channel, urpc_msg_t *msg) {
    if (!channel || !msg) {
        return -1;
    }

    uint32_t local_core = hal_cpu_get_id();

    // 1. Validate receiver matches local core
    if (msg->receiver_core_id != local_core || channel->receiver_core_id != local_core) {
        return -1;
    }

    // 2. Validate sender matches channel's expected sender
    if (msg->sender_core_id != channel->sender_core_id) {
        return -1;
    }

    // 3. Validate channel direction consistency
    if (channel->sender_core_id == channel->receiver_core_id) {
        return -1; // Intra-core messages over URPC shouldn't happen here
    }

    // 4. Validate header/payload sizes
    if (msg->payload_size > sizeof(msg->payload_data)) {
        return -1;
    }

    // 5. Authorize message
    if (mk_authorize_message(channel, msg) != 0) {
        return -1;
    }

    // 6. Action / routing
    if (msg->msg_type == MK_MSG_TYPE_AI_SUGGESTION) {
        /* Scheduler control-plane hook placeholder. */
    } else {
        sched_notify_ipc_ready(local_core, msg->msg_type);
    }

    return 0;
}
