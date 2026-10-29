#include "ipc_endpoint.h"
#include "kernel_safety.h"
#include "cap_policy.h"
#include "spinlock.h"

#include <stddef.h>
#include <stdint.h>

typedef struct {
    uint8_t in_use;
    ipc_endpoint_state_t state;
    uint32_t generation;

    // Counters
    uint32_t queued_msgs;
    uint32_t dropped_msgs;
    uint32_t timeouts;

    ipc_message_t msg;
    uint8_t has_msg;
    /* Pointer-free pending transfer; has_msg is 0 empty, 1 queued, 2 claimed.
     * Endpoint lock protects message/claim state. CSpace installation runs only
     * after releasing it; no endpoint lock spans a capability operation. */
    bh_cap_locator_t cap_transfer_source;
    spinlock_t lock;
    wait_queue_t senders;
    wait_queue_t receivers;
} ipc_endpoint_t;

/* Transitional lock-protected shared pool. Allocation is not yet serialized
 * across cores and no endpoint close/reclamation mechanism exists. This patch
 * secures pending transfer storage, not the complete endpoint object lifetime. */
static ipc_endpoint_t g_endpoints[BHARAT_IPC_MAX_ENDPOINTS];

static uint64_t ipc_wait_deadline(uint64_t timeout_ticks) {
    if (timeout_ticks == 0U || timeout_ticks == UINT64_MAX) return 0U;
    uint64_t now = sched_get_ticks();
    return timeout_ticks > UINT64_MAX - now ? UINT64_MAX : now + timeout_ticks;
}

static ipc_endpoint_t* endpoint_by_ref(uint64_t ref) {
    uint32_t idx = ref & 0xFFFFFFFF;
    uint32_t gen = ref >> 32;

    if (idx >= BHARAT_ARRAY_SIZE(g_endpoints)) {
        return NULL;
    }
    if (g_endpoints[idx].in_use == 0U) {
        return NULL;
    }
    if (g_endpoints[idx].generation != gen) {
        return NULL; // Stale endpoint
    }
    return &g_endpoints[idx];
}

int ipc_endpoint_create(capability_table_t* table, uint32_t* out_send_cap, uint32_t* out_recv_cap) {
    if (!table || !out_send_cap || !out_recv_cap) {
        return IPC_ERR_INVALID;
    }

    for (size_t i = 0; i < BHARAT_ARRAY_SIZE(g_endpoints); ++i) {
        if (g_endpoints[i].in_use == 0U) {
            g_endpoints[i].in_use = 1U;
            g_endpoints[i].state = IPC_ENDPOINT_STATE_READY;
            g_endpoints[i].generation++; // Generation changes on rebind/recreate
            g_endpoints[i].queued_msgs = 0U;
            g_endpoints[i].dropped_msgs = 0U;
            g_endpoints[i].timeouts = 0U;
            g_endpoints[i].has_msg = 0U;
            g_endpoints[i].msg.msg_len = 0U;
            spin_lock_init(&g_endpoints[i].lock);
            sched_wait_queue_init(&g_endpoints[i].senders);
            sched_wait_queue_init(&g_endpoints[i].receivers);

            // The object ref needs to encode generation for the handle later if we want the endpoint generation
            uint64_t obj_ref = i | ((uint64_t)g_endpoints[i].generation << 32);

            if (cap_table_grant(table, CAP_TYPE_ENDPOINT, obj_ref, CAP_RIGHT_ENDPOINT_SEND | CAP_RIGHT_DELEGATE, out_send_cap) != 0) {
                g_endpoints[i].in_use = 0U;
                g_endpoints[i].state = IPC_ENDPOINT_STATE_FREE;
                return IPC_ERR_NO_SPACE;
            }

            if (cap_table_grant(table, CAP_TYPE_ENDPOINT, obj_ref, CAP_RIGHT_ENDPOINT_RECEIVE | CAP_RIGHT_DELEGATE, out_recv_cap) != 0) {
                cap_table_revoke(table, *out_send_cap);
                g_endpoints[i].in_use = 0U;
                g_endpoints[i].state = IPC_ENDPOINT_STATE_FREE;
                return IPC_ERR_NO_SPACE;
            }

            return IPC_OK;
        }
    }

    return IPC_ERR_NO_SPACE;
}

int ipc_endpoint_send(capability_table_t* table, uint32_t send_cap, const void* payload, uint32_t payload_len, uint64_t timeout_ticks, uint32_t cap_to_send, uint64_t cap_send_rights) {
    if (!table || !payload || payload_len == 0U) {
        return IPC_ERR_INVALID;
    }

    capability_entry_t e = {0};
    if (cap_table_lookup(table, send_cap, CAP_TYPE_ENDPOINT, CAP_RIGHT_ENDPOINT_SEND, &e) != 0) {
        return IPC_ERR_PERM;
    }

    ipc_endpoint_t* ep = endpoint_by_ref(e.object_ref);
    if (!ep) {
        return IPC_ERR_INVALID;
    }

    if (payload_len > sizeof(ep->msg.payload)) {
        return IPC_ERR_INVALID;
    }

    // Only a checked locator is retained across send/receive or sender exit.
    bh_cap_locator_t transfer_source = {0};
    if (cap_to_send != 0U) {
        if (bh_cap_transfer_capture(table, cap_to_send, cap_send_rights,
                                    &transfer_source) != K_OK) {
            return IPC_ERR_CAP_TRANSFER_NOT_ALLOWED;
        }
    }

    const uint64_t deadline = ipc_wait_deadline(timeout_ticks);
    for (;;) {
        /* Blocking never preserves authorization: recheck the caller's current
         * generation/rights after every wake before touching the endpoint. */
        capability_entry_t current_endpoint = {0};
        if (cap_table_lookup(table, send_cap, CAP_TYPE_ENDPOINT,
                             CAP_RIGHT_ENDPOINT_SEND, &current_endpoint) != 0 ||
            current_endpoint.object_ref != e.object_ref) {
            return IPC_ERR_PERM;
        }
        if (cap_to_send != 0U &&
            bh_cap_transfer_capture(table, cap_to_send, cap_send_rights,
                                    &transfer_source) != K_OK) {
            return IPC_ERR_CAP_TRANSFER_NOT_ALLOWED;
        }
        spin_lock(&ep->lock);

        if (ep->state != IPC_ENDPOINT_STATE_READY) {
            spin_unlock(&ep->lock);
            return IPC_ERR_CLOSED;
        }

        if (ep->has_msg == 0U) {
            break;
        }
        if (timeout_ticks == 0) {
            ep->dropped_msgs++;
            spin_unlock(&ep->lock);
            return IPC_ERR_WOULD_BLOCK;
        }

        if (timeout_ticks != UINT64_MAX && sched_get_ticks() >= deadline) {
            ep->timeouts++;
            spin_unlock(&ep->lock);
            return IPC_ERR_TIMEOUT;
        }
        bh_thread_t* cur = sched_current_thread();
        if (!cur) {
            spin_unlock(&ep->lock);
            return IPC_ERR_WOULD_BLOCK;
        }
        cur->ipc_wakeup_reason = IPC_OK;
        if (timeout_ticks != UINT64_MAX) {
            cur->ipc_deadline_ticks = deadline;
        } else {
            cur->ipc_deadline_ticks = 0;
        }
        sched_wait_queue_enqueue(&ep->senders, cur);
        ep->queued_msgs++;
        spin_unlock(&ep->lock);
        sched_block();

        if (cur->ipc_wakeup_reason != IPC_OK) {
            if (cur->ipc_wakeup_reason == IPC_ERR_TIMEOUT) {
                spin_lock(&ep->lock);
                ep->timeouts++;
                ep->queued_msgs--;
                spin_unlock(&ep->lock);
            }
            return cur->ipc_wakeup_reason;
        } else {
            spin_lock(&ep->lock);
            ep->queued_msgs--;
            spin_unlock(&ep->lock);
        }
    }

    const uint8_t* src = (const uint8_t*)payload;
    for (uint32_t i = 0; i < payload_len; ++i) {
        ep->msg.payload[i] = src[i];
    }
    ep->msg.msg_len = payload_len;

    // Store capability transfer metadata
    if (cap_to_send != 0U) {
        ep->msg.cap_transfer_id = cap_to_send;
        ep->msg.cap_transfer_rights = cap_send_rights;
        ep->cap_transfer_source = transfer_source;
    } else {
        ep->msg.cap_transfer_id = 0U;
        ep->msg.cap_transfer_rights = 0U;
        ep->cap_transfer_source = (bh_cap_locator_t){0};
    }

    ep->has_msg = 1U;

    bh_thread_t* recv = sched_wait_queue_dequeue(&ep->receivers);
    spin_unlock(&ep->lock);

    if (recv != NULL) {
        recv->ipc_wakeup_reason = IPC_OK;
        recv->ipc_deadline_ticks = 0;
        sched_wakeup(recv);
    }

    return IPC_OK;
}

int ipc_endpoint_receive(capability_table_t* table, uint32_t recv_cap, void* out_payload, uint32_t out_payload_capacity, uint32_t* out_received_len, uint64_t timeout_ticks, uint32_t* out_received_cap) {
    if (!table || !out_payload || out_payload_capacity == 0U || !out_received_len) {
        return IPC_ERR_INVALID;
    }

    *out_received_len = 0U;
    if (out_received_cap != NULL) {
        *out_received_cap = 0U;
    }
    capability_entry_t e = {0};
    if (cap_table_lookup(table, recv_cap, CAP_TYPE_ENDPOINT, CAP_RIGHT_ENDPOINT_RECEIVE, &e) != 0) {
        return IPC_ERR_PERM;
    }

    ipc_endpoint_t* ep = endpoint_by_ref(e.object_ref);
    if (!ep) {
        return IPC_ERR_INVALID;
    }

    const uint64_t deadline = ipc_wait_deadline(timeout_ticks);
    for (;;) {
        capability_entry_t current_endpoint = {0};
        if (cap_table_lookup(table, recv_cap, CAP_TYPE_ENDPOINT,
                             CAP_RIGHT_ENDPOINT_RECEIVE, &current_endpoint) != 0 ||
            current_endpoint.object_ref != e.object_ref) {
            return IPC_ERR_PERM;
        }
        spin_lock(&ep->lock);

        if (ep->state != IPC_ENDPOINT_STATE_READY) {
            spin_unlock(&ep->lock);
            return IPC_ERR_CLOSED;
        }

        if (ep->has_msg == 1U) {
            break;
        }
        if (timeout_ticks == 0) {
            spin_unlock(&ep->lock);
            return IPC_ERR_WOULD_BLOCK;
        }

        if (timeout_ticks != UINT64_MAX && sched_get_ticks() >= deadline) {
            ep->timeouts++;
            spin_unlock(&ep->lock);
            return IPC_ERR_TIMEOUT;
        }
        bh_thread_t* cur = sched_current_thread();
        if (!cur) {
            spin_unlock(&ep->lock);
            return IPC_ERR_WOULD_BLOCK;
        }
        cur->ipc_wakeup_reason = IPC_OK;
        if (timeout_ticks != UINT64_MAX) {
            cur->ipc_deadline_ticks = deadline;
        } else {
            cur->ipc_deadline_ticks = 0;
        }
        sched_wait_queue_enqueue(&ep->receivers, cur);
        ep->queued_msgs++;
        spin_unlock(&ep->lock);
        sched_block();

        if (cur->ipc_wakeup_reason != IPC_OK) {
            if (cur->ipc_wakeup_reason == IPC_ERR_TIMEOUT) {
                spin_lock(&ep->lock);
                ep->timeouts++;
                ep->queued_msgs--;
                spin_unlock(&ep->lock);
            }
            return cur->ipc_wakeup_reason;
        } else {
            spin_lock(&ep->lock);
            ep->queued_msgs--;
            spin_unlock(&ep->lock);
        }
    }

    if (out_payload_capacity < ep->msg.msg_len) {
        spin_unlock(&ep->lock);
        return IPC_ERR_NO_SPACE;
    }

    // Claim exactly one message, then leave the endpoint lock before entering
    // CSpace code. Other receivers see a busy slot until reconciliation below.
    ep->has_msg = 2U;
    bh_cap_locator_t source = ep->cap_transfer_source;
    uint64_t transfer_rights = ep->msg.cap_transfer_rights;
    uint32_t transfer_id = ep->msg.cap_transfer_id;
    spin_unlock(&ep->lock);

    uint32_t newly_installed_cap_id = 0U;
    kstatus_t install_ret = K_OK;
    if (transfer_id != 0U && out_received_cap != NULL) {
        install_ret = bh_cap_transfer_install(&source, table, transfer_rights,
                                              &newly_installed_cap_id);
    }

    /* The pool has no destroy/rebind path today, so a claimed endpoint cannot
     * disappear while unlocked. A future close operation must fence this claim
     * and compensate any installed capability before reusing its generation. */
    spin_lock(&ep->lock);
    if (install_ret == K_ERR_NO_RESOURCES) {
        // Receiver table saturation is retryable; preserve the pending message.
        ep->has_msg = 1U;
        bh_thread_t *receiver = sched_wait_queue_dequeue(&ep->receivers);
        spin_unlock(&ep->lock);
        if (receiver != NULL) {
            receiver->ipc_wakeup_reason = IPC_OK;
            receiver->ipc_deadline_ticks = 0U;
            sched_wakeup(receiver);
        }
        return IPC_ERR_CAP_INSTALL_FAILED;
    }
    if (install_ret != K_OK) {
        // Stale/revoked/destroyed/foreign sources can never become valid again.
        // Discard rather than permanently wedging the one-message endpoint.
        ep->msg.msg_len = 0U;
        ep->msg.cap_transfer_id = 0U;
        ep->msg.cap_transfer_rights = 0U;
        ep->cap_transfer_source = (bh_cap_locator_t){0};
        ep->has_msg = 0U;
        bh_thread_t *sender = sched_wait_queue_dequeue(&ep->senders);
        spin_unlock(&ep->lock);
        if (sender != NULL) {
            sender->ipc_wakeup_reason = IPC_OK;
            sender->ipc_deadline_ticks = 0;
            sched_wakeup(sender);
        }
        return IPC_ERR_CAP_INSTALL_FAILED;
    }

    // Phase 2: Commit message consumption
    uint8_t* dst = (uint8_t*)out_payload;
    for (uint32_t i = 0; i < ep->msg.msg_len; ++i) {
        dst[i] = ep->msg.payload[i];
    }

    *out_received_len = ep->msg.msg_len;
    if (out_received_cap) {
        *out_received_cap = newly_installed_cap_id;
    }

    ep->msg.msg_len = 0U;
    ep->msg.cap_transfer_id = 0U;
    ep->msg.cap_transfer_rights = 0U;
    ep->cap_transfer_source = (bh_cap_locator_t){0};
    ep->has_msg = 0U;

    bh_thread_t* sender = sched_wait_queue_dequeue(&ep->senders);
    spin_unlock(&ep->lock);

    if (sender != NULL) {
        sender->ipc_wakeup_reason = IPC_OK;
        sender->ipc_deadline_ticks = 0;
        sched_wakeup(sender);
    }

    return IPC_OK;
}
