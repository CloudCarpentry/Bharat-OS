#include "ipc_endpoint.h"
#include "kernel_safety.h"

#include <stddef.h>
#include <stdint.h>

#define MAX_ENDPOINTS 32U

typedef struct {
    uint8_t in_use;
    ipc_message_t msg;
    uint8_t has_msg;
    wait_queue_t senders;
    wait_queue_t receivers;
} ipc_endpoint_t;

static ipc_endpoint_t g_endpoints[MAX_ENDPOINTS];

static ipc_endpoint_t* endpoint_by_ref(uint64_t ref) {
    if (ref >= BHARAT_ARRAY_SIZE(g_endpoints)) {
        return NULL;
    }
    if (g_endpoints[ref].in_use == 0U) {
        return NULL;
    }
    return &g_endpoints[ref];
}

int ipc_endpoint_create(capability_table_t* table, uint32_t* out_send_cap, uint32_t* out_recv_cap) {
    if (!table || !out_send_cap || !out_recv_cap) {
        return IPC_ERR_INVALID;
    }

    for (size_t i = 0; i < BHARAT_ARRAY_SIZE(g_endpoints); ++i) {
        if (g_endpoints[i].in_use == 0U) {
            g_endpoints[i].in_use = 1U;
            g_endpoints[i].has_msg = 0U;
            g_endpoints[i].msg.msg_len = 0U;
            sched_wait_queue_init(&g_endpoints[i].senders);
            sched_wait_queue_init(&g_endpoints[i].receivers);

            if (cap_table_grant(table, CAP_OBJ_ENDPOINT, i, CAP_PERM_SEND | CAP_PERM_DELEGATE, out_send_cap) != 0) {
                g_endpoints[i].in_use = 0U;
                return IPC_ERR_NO_SPACE;
            }

            if (cap_table_grant(table, CAP_OBJ_ENDPOINT, i, CAP_PERM_RECEIVE | CAP_PERM_DELEGATE, out_recv_cap) != 0) {
                g_endpoints[i].in_use = 0U;
                return IPC_ERR_NO_SPACE;
            }

            return IPC_OK;
        }
    }

    return IPC_ERR_NO_SPACE;
}

int ipc_endpoint_send(capability_table_t* table, uint32_t send_cap, const void* payload, uint32_t payload_len, uint64_t timeout_ticks) {
    if (!table || !payload || payload_len == 0U) {
        return IPC_ERR_INVALID;
    }

    capability_entry_t e = {0};
    if (cap_table_lookup(table, send_cap, CAP_OBJ_ENDPOINT, CAP_PERM_SEND, &e) != 0) {
        return IPC_ERR_PERM;
    }

    ipc_endpoint_t* ep = endpoint_by_ref(e.object_ref);
    if (!ep) {
        return IPC_ERR_INVALID;
    }

    if (payload_len > sizeof(ep->msg.payload)) {
        return IPC_ERR_INVALID;
    }

    if (ep->has_msg != 0U) {
        if (timeout_ticks == 0) {
            return IPC_ERR_WOULD_BLOCK;
        }

        kthread_t* cur = sched_current_thread();
        if (cur) {
            cur->ipc_wakeup_reason = IPC_OK;
            if (timeout_ticks != UINT64_MAX) {
                cur->ipc_deadline_ticks = sched_get_ticks() + timeout_ticks;
            } else {
                cur->ipc_deadline_ticks = 0;
            }
            sched_wait_queue_enqueue(&ep->senders, cur);
            sched_block();

            if (cur->ipc_wakeup_reason != IPC_OK) {
                return cur->ipc_wakeup_reason;
            }
        } else {
            return IPC_ERR_WOULD_BLOCK;
        }
    }

    const uint8_t* src = (const uint8_t*)payload;
    for (uint32_t i = 0; i < payload_len; ++i) {
        ep->msg.payload[i] = src[i];
    }
    ep->msg.msg_len = payload_len;
    ep->has_msg = 1U;

    kthread_t* recv = sched_wait_queue_dequeue(&ep->receivers);
    if (recv) {
        recv->ipc_wakeup_reason = IPC_OK;
        recv->ipc_deadline_ticks = 0;
        sched_wakeup(recv);
    }

    return IPC_OK;
}

int ipc_endpoint_receive(capability_table_t* table, uint32_t recv_cap, void* out_payload, uint32_t out_payload_capacity, uint32_t* out_received_len, uint64_t timeout_ticks) {
    if (!table || !out_payload || out_payload_capacity == 0U || !out_received_len) {
        return IPC_ERR_INVALID;
    }

    capability_entry_t e = {0};
    if (cap_table_lookup(table, recv_cap, CAP_OBJ_ENDPOINT, CAP_PERM_RECEIVE, &e) != 0) {
        return IPC_ERR_PERM;
    }

    ipc_endpoint_t* ep = endpoint_by_ref(e.object_ref);
    if (!ep) {
        return IPC_ERR_INVALID;
    }

    if (ep->has_msg == 0U) {
        if (timeout_ticks == 0) {
            return IPC_ERR_WOULD_BLOCK;
        }

        kthread_t* cur = sched_current_thread();
        if (cur) {
            cur->ipc_wakeup_reason = IPC_OK;
            if (timeout_ticks != UINT64_MAX) {
                cur->ipc_deadline_ticks = sched_get_ticks() + timeout_ticks;
            } else {
                cur->ipc_deadline_ticks = 0;
            }
            sched_wait_queue_enqueue(&ep->receivers, cur);
            sched_block();

            if (cur->ipc_wakeup_reason != IPC_OK) {
                return cur->ipc_wakeup_reason;
            }
        } else {
            return IPC_ERR_WOULD_BLOCK;
        }
    }

    if (out_payload_capacity < ep->msg.msg_len) {
        return IPC_ERR_NO_SPACE;
    }

    uint8_t* dst = (uint8_t*)out_payload;
    for (uint32_t i = 0; i < ep->msg.msg_len; ++i) {
        dst[i] = ep->msg.payload[i];
    }

    *out_received_len = ep->msg.msg_len;
    ep->msg.msg_len = 0U;
    ep->has_msg = 0U;

    kthread_t* sender = sched_wait_queue_dequeue(&ep->senders);
    if (sender) {
        sender->ipc_wakeup_reason = IPC_OK;
        sender->ipc_deadline_ticks = 0;
        sched_wakeup(sender);
    }

    return IPC_OK;
}
