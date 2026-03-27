#include "../../include/ipc/mk_dispatch.h"
#include "../../include/hal/hal.h"
#include "sched/sched.h"

#define MK_MSG_TYPE_AI_SUGGESTION 1U

// Simple first-pass authorization stub
static int mk_authorize_message(mk_channel_t *channel, urpc_msg_t *msg) {
    (void)channel;
    // For now, accept generic safe types but reject unrecognized or sensitive types implicitly
    // This will be expanded when full distributed capabilities and tokens are ready.
    if (msg->type == MK_MSG_TYPE_AI_SUGGESTION) {
        return 0; // Temporarily allow AI suggestions until proper policies are wired
    }

    if (msg->type == MK_MSG_THREAD_HANDOFF_REQ) {
        // Mock capability check: Ensure sender provided a valid schedule authority token
        // In a real implementation, this would lookup the auth_token in the receiver's CNode
        // to verify it grants CAP_SCHED_TARGET on this core.
        if (msg->auth_token == 0) {
            return -1; // Denied: Missing capability token
        }
        return 0;
    }

    // Default route: allow control messages but this will tighten
    return 0;
}

static void mk_handle_thread_handoff_req(mk_channel_t *channel, urpc_msg_t *msg) {
    if (msg->payload_size != sizeof(mk_msg_thread_handoff_t)) {
        // Send NACK for invalid size
        urpc_msg_t nack = {
            .type = MK_MSG_THREAD_HANDOFF_NACK,
            .payload_size = 0,
            .src_core = msg->dst_core,
            .dst_core = msg->src_core,
            .msg_id = msg->msg_id
        };
        mk_send_message(channel, nack.type, nack.payload_data, nack.payload_size);
        return;
    }

    mk_msg_thread_handoff_t payload;
    __builtin_memcpy(&payload, msg->payload_data, sizeof(payload));

    uint32_t local_core = hal_cpu_get_id();

    if (payload.target_core != local_core) {
        // Bad core routing
        urpc_msg_t nack = {
            .type = MK_MSG_THREAD_HANDOFF_NACK,
            .payload_size = 0,
            .src_core = local_core,
            .dst_core = msg->src_core,
            .msg_id = msg->msg_id
        };
        mk_send_message(channel, nack.type, nack.payload_data, nack.payload_size);
        return;
    }

    kthread_t *thread = sched_find_thread_by_id(payload.thread_id);
    if (!thread) {
        // Thread not found
        urpc_msg_t nack = {
            .type = MK_MSG_THREAD_HANDOFF_NACK,
            .payload_size = 0,
            .src_core = local_core,
            .dst_core = msg->src_core,
            .msg_id = msg->msg_id
        };
        mk_send_message(channel, nack.type, nack.payload_data, nack.payload_size);
        return;
    }

    // Spinlock/IRQ disable to protect thread state mutation
    hal_cpu_disable_interrupts();

    if (thread->state != THREAD_STATE_REMOTE_HANDOFF_PENDING) {
        hal_cpu_enable_interrupts();
        urpc_msg_t nack = {
            .type = MK_MSG_THREAD_HANDOFF_NACK,
            .payload_size = 0,
            .src_core = local_core,
            .dst_core = msg->src_core,
            .msg_id = msg->msg_id
        };
        mk_send_message(channel, nack.type, nack.payload_data, nack.payload_size);
        return;
    }

    // Take ownership
    thread->bound_core_id = local_core;
    // Affinity mask should already encompass this core if handoff was requested,
    // but ensuring it's valid is good practice.

    // Enqueue locally (sched_enqueue transitions to READY and adds to local rq)
    // using the remote inbox if we want to be clean, but since we are the local core
    // and hold no rq locks yet, we can just call sched_enqueue.
    sched_enqueue(thread, local_core);

    hal_cpu_enable_interrupts();

    // Send ACK
    urpc_msg_t ack = {
        .type = MK_MSG_THREAD_HANDOFF_ACK,
        .payload_size = 0,
        .src_core = local_core,
        .dst_core = msg->src_core,
        .msg_id = msg->msg_id
    };
    mk_send_message(channel, ack.type, ack.payload_data, ack.payload_size);
}

int mk_dispatch_message(mk_channel_t *channel, urpc_msg_t *msg) {
    if (!channel || !msg) {
        return -1;
    }

    uint32_t local_core = hal_cpu_get_id();

    // 1. Validate receiver matches local core
    if (msg->dst_core != local_core || channel->dst_core != local_core) {
        return -1;
    }

    // 2. Validate sender matches channel's expected sender
    if (msg->src_core != channel->src_core) {
        return -1;
    }

    // 3. Validate channel direction consistency
    if (channel->src_core == channel->dst_core) {
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
    switch (msg->type) {
        case MK_MSG_TYPE_ACK:
        case MK_MSG_TYPE_NACK:
            // Route to transaction table for ACK/NACK completion
            break;

        case MK_MSG_FRAME_ALLOC_REQ:
        case MK_MSG_FRAME_FREE_REQ:
        case MK_MSG_FRAME_MAP_REQ:
        case MK_MSG_FRAME_UNMAP_REQ:
            // Route to memory manager ownership RPC handlers
            break;

        case MK_MSG_PROC_CREATE_REQ:
        case MK_MSG_PROC_DESTROY_REQ:
        case MK_MSG_PROC_SIGNAL_REQ:
            // Route to process manager ownership RPC handlers
            break;

        case MK_MSG_ASPACE_CREATE_REQ:
        case MK_MSG_ASPACE_MAP_REQ:
        case MK_MSG_ASPACE_PROTECT_REQ:
        case MK_MSG_ASPACE_UNMAP_REQ:
            // Route to address space manager ownership RPC handlers
            break;

        case MK_MSG_CAP_GRANT_REQ:
        case MK_MSG_CAP_REVOKE_REQ:
        case MK_MSG_CAP_DERIVE_REQ:
        case MK_MSG_CAP_LOOKUP_REQ:
            // Route to capability manager ownership RPC handlers
            break;

        case MK_MSG_TYPE_AI_SUGGESTION:
            /* Scheduler control-plane hook placeholder. */
            break;

        case MK_MSG_THREAD_HANDOFF_REQ:
            mk_handle_thread_handoff_req(channel, msg);
            break;

        case MK_MSG_THREAD_HANDOFF_ACK:
        case MK_MSG_THREAD_HANDOFF_NACK:
            /* Expected replies for handoffs, tracked by L1 protocol/transactions later */
            break;

        default:
            sched_notify_ipc_ready(local_core, msg->type);
            break;
    }

    return 0;
}
