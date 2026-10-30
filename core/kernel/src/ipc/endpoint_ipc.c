#include "ipc_endpoint.h"
#include "sched/sched.h"
#include "kernel_safety.h"
#include "cap_policy.h"
#include "spinlock.h"
#include "bharat_config.h"
#include "hal/hal.h"

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifndef MAX_CPUS
#ifdef MAX_SUPPORTED_CORES
#define MAX_CPUS MAX_SUPPORTED_CORES
#else
#define MAX_CPUS 32U
#endif
#endif

typedef struct {
    uint8_t in_use;
    ipc_endpoint_state_t state;
    uint32_t generation;
    uint32_t owner_core;

    /* Explicit reference & claim tracking (Amendment B & C) */
    uint32_t send_cap_refs;
    uint32_t recv_cap_refs;
    uint32_t active_claims;

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

    /* Second transfer slot for explicit RPC reply send capability (Amendment D) */
    uint32_t reply_cap_transfer_id;
    uint64_t reply_cap_transfer_rights;
    bh_cap_locator_t reply_cap_transfer_source;

    ipc_provenance_t provenance;
    spinlock_t lock;
    wait_queue_t senders;
    wait_queue_t receivers;
} ipc_endpoint_t;

/* Per-core partitioned endpoint pool. Lifecycle state changes are owned by owner_core. */
static ipc_endpoint_t g_endpoints[MAX_CPUS][BHARAT_IPC_ENDPOINTS_PER_CORE];

static inline uint64_t endpoint_make_ref(uint32_t owner_core, uint32_t slot, uint32_t generation) {
    return ((uint64_t)(owner_core & 0xFFFFU) << 48) |
           ((uint64_t)generation << 16) |
           (uint64_t)(slot & 0xFFFFU);
}

static inline void endpoint_unpack_ref(uint64_t ref, uint32_t *out_core, uint32_t *out_slot, uint32_t *out_gen) {
    if (out_core) *out_core = (uint32_t)((ref >> 48) & 0xFFFFU);
    if (out_gen)  *out_gen  = (uint32_t)((ref >> 16) & 0xFFFFFFFFU);
    if (out_slot) *out_slot = (uint32_t)(ref & 0xFFFFU);
}

static uint64_t ipc_wait_deadline(uint64_t timeout_ticks) {
    if (timeout_ticks == 0U || timeout_ticks == UINT64_MAX) return 0U;
    uint64_t now = sched_get_ticks();
    return timeout_ticks > UINT64_MAX - now ? UINT64_MAX : now + timeout_ticks;
}

static ipc_endpoint_t* endpoint_by_ref(uint64_t ref) {
    uint32_t core = 0U, slot = 0U, gen = 0U;
    endpoint_unpack_ref(ref, &core, &slot, &gen);

    if (core >= MAX_CPUS || slot >= BHARAT_IPC_ENDPOINTS_PER_CORE) {
        return NULL;
    }
    ipc_endpoint_t *ep = &g_endpoints[core][slot];
    if (ep->in_use == 0U) {
        return NULL;
    }
    if (ep->generation != gen) {
        return NULL; // Stale endpoint
    }
    return ep;
}

static bool ipc_wait_queue_remove(wait_queue_t *queue, bh_thread_t *target) {
    if (!queue || !target || !queue->head) {
        return false;
    }
    bh_thread_t *prev = NULL;
    bh_thread_t *curr = queue->head;
    while (curr != NULL) {
        if (curr == target) {
            if (prev != NULL) {
                prev->next_waiter = curr->next_waiter;
            } else {
                queue->head = curr->next_waiter;
            }
            if (queue->tail == curr) {
                queue->tail = prev;
            }
            curr->next_waiter = NULL;
            return true;
        }
        prev = curr;
        curr = curr->next_waiter;
    }
    return false;
}

static void ipc_endpoint_clear_msg_locked(ipc_endpoint_t *ep) {
    ep->msg.msg_len = 0U;
    ep->msg.cap_transfer_id = 0U;
    ep->msg.cap_transfer_rights = 0U;
    ep->cap_transfer_source = (bh_cap_locator_t){0};
    ep->reply_cap_transfer_id = 0U;
    ep->reply_cap_transfer_rights = 0U;
    ep->reply_cap_transfer_source = (bh_cap_locator_t){0};
    ep->provenance = (ipc_provenance_t){0};
    ep->has_msg = 0U;
}

/* Must be called with ep->lock held. Reclaims the slot only after all capability
 * references, in-flight claims, and wait queues are settled. */
static void ipc_endpoint_try_reclaim_locked(ipc_endpoint_t *ep) {
    if (!ep || ep->in_use == 0U) {
        return;
    }
    if (ep->state != IPC_ENDPOINT_STATE_CLOSED &&
        ep->state != IPC_ENDPOINT_STATE_CLOSING &&
        ep->state != IPC_ENDPOINT_STATE_REVOKED) {
        return;
    }
    if (ep->send_cap_refs == 0U &&
        ep->recv_cap_refs == 0U &&
        ep->active_claims == 0U &&
        ep->senders.head == NULL &&
        ep->receivers.head == NULL) {
        ipc_endpoint_clear_msg_locked(ep);
        ep->state = IPC_ENDPOINT_STATE_FREE;
        ep->generation++;
        if (ep->generation == 0U) {
            ep->generation = 1U;
        }
        ep->in_use = 0U;
    }
}

int ipc_endpoint_acquire_send_cap(uint64_t endpoint_obj_ref) {
    ipc_endpoint_t *ep = endpoint_by_ref(endpoint_obj_ref);
    if (!ep) return IPC_ERR_INVALID;
    spin_lock(&ep->lock);
    if (ep->in_use == 0U || ep->state != IPC_ENDPOINT_STATE_READY) {
        spin_unlock(&ep->lock);
        return IPC_ERR_CLOSED;
    }
    ep->send_cap_refs++;
    spin_unlock(&ep->lock);
    return IPC_OK;
}

int ipc_endpoint_acquire_recv_cap(uint64_t endpoint_obj_ref) {
    ipc_endpoint_t *ep = endpoint_by_ref(endpoint_obj_ref);
    if (!ep) return IPC_ERR_INVALID;
    spin_lock(&ep->lock);
    if (ep->in_use == 0U || ep->state != IPC_ENDPOINT_STATE_READY) {
        spin_unlock(&ep->lock);
        return IPC_ERR_CLOSED;
    }
    ep->recv_cap_refs++;
    spin_unlock(&ep->lock);
    return IPC_OK;
}

int ipc_endpoint_release_send_cap(uint64_t endpoint_obj_ref) {
    ipc_endpoint_t *ep = endpoint_by_ref(endpoint_obj_ref);
    if (!ep) return IPC_ERR_INVALID;
    spin_lock(&ep->lock);
    if (ep->send_cap_refs > 0U) {
        ep->send_cap_refs--;
    }
    if (ep->send_cap_refs == 0U && ep->recv_cap_refs == 0U &&
        ep->state == IPC_ENDPOINT_STATE_READY) {
        ep->state = IPC_ENDPOINT_STATE_CLOSED;
    }
    ipc_endpoint_try_reclaim_locked(ep);
    spin_unlock(&ep->lock);
    return IPC_OK;
}

int ipc_endpoint_release_recv_cap(uint64_t endpoint_obj_ref) {
    ipc_endpoint_t *ep = endpoint_by_ref(endpoint_obj_ref);
    if (!ep) return IPC_ERR_INVALID;

    bh_thread_t *wake_list = NULL;
    spin_lock(&ep->lock);
    if (ep->recv_cap_refs > 0U) {
        ep->recv_cap_refs--;
    }
    /* Only transition to CLOSED if NO receive capabilities remain */
    if (ep->recv_cap_refs == 0U && ep->state == IPC_ENDPOINT_STATE_READY) {
        ep->state = IPC_ENDPOINT_STATE_CLOSED;
        if (ep->has_msg == 1U) {
            ipc_endpoint_clear_msg_locked(ep);
        }
        bh_thread_t *s = NULL;
        while ((s = sched_wait_queue_dequeue(&ep->senders)) != NULL) {
            s->ipc_wakeup_reason = IPC_ERR_CLOSED;
            s->ipc_deadline_ticks = 0U;
            s->next_waiter = wake_list;
            wake_list = s;
        }
        bh_thread_t *r = NULL;
        while ((r = sched_wait_queue_dequeue(&ep->receivers)) != NULL) {
            r->ipc_wakeup_reason = IPC_ERR_CLOSED;
            r->ipc_deadline_ticks = 0U;
            r->next_waiter = wake_list;
            wake_list = r;
        }
    }
    ipc_endpoint_try_reclaim_locked(ep);
    spin_unlock(&ep->lock);

    while (wake_list != NULL) {
        bh_thread_t *next = wake_list->next_waiter;
        wake_list->next_waiter = NULL;
        sched_wakeup(wake_list);
        wake_list = next;
    }
    return IPC_OK;
}

int ipc_endpoint_owner_close(capability_table_t *table, uint32_t cap_id) {
    if (!table || cap_id == 0U) {
        return IPC_ERR_INVALID;
    }
    capability_entry_t e = {0};
    if (cap_table_lookup(table, cap_id, CAP_TYPE_ENDPOINT, CAP_RIGHT_ENDPOINT_RECEIVE, &e) != 0) {
        return IPC_ERR_PERM;
    }
    ipc_endpoint_t *ep = endpoint_by_ref(e.object_ref);
    if (!ep) {
        return IPC_ERR_INVALID;
    }
    if (ep->owner_core != hal_cpu_get_id()) {
        return IPC_ERR_PERM;
    }

    bh_thread_t *wake_list = NULL;
    spin_lock(&ep->lock);
    if (ep->state == IPC_ENDPOINT_STATE_READY || ep->state == IPC_ENDPOINT_STATE_CLOSING) {
        ep->state = IPC_ENDPOINT_STATE_CLOSING;
        if (ep->has_msg == 1U) {
            ipc_endpoint_clear_msg_locked(ep);
        }
        bh_thread_t *s = NULL;
        while ((s = sched_wait_queue_dequeue(&ep->senders)) != NULL) {
            s->ipc_wakeup_reason = IPC_ERR_CLOSED;
            s->ipc_deadline_ticks = 0U;
            s->next_waiter = wake_list;
            wake_list = s;
        }
        bh_thread_t *r = NULL;
        while ((r = sched_wait_queue_dequeue(&ep->receivers)) != NULL) {
            r->ipc_wakeup_reason = IPC_ERR_CLOSED;
            r->ipc_deadline_ticks = 0U;
            r->next_waiter = wake_list;
            wake_list = r;
        }
        ep->state = IPC_ENDPOINT_STATE_CLOSED;
    }
    ipc_endpoint_try_reclaim_locked(ep);
    spin_unlock(&ep->lock);

    while (wake_list != NULL) {
        bh_thread_t *next = wake_list->next_waiter;
        wake_list->next_waiter = NULL;
        sched_wakeup(wake_list);
        wake_list = next;
    }
    return IPC_OK;
}

int ipc_endpoint_cancel_waiter(uint64_t endpoint_obj_ref, bh_thread_t *thread, int cancel_reason) {
    if (!thread) return IPC_ERR_INVALID;
    ipc_endpoint_t *ep = endpoint_by_ref(endpoint_obj_ref);
    if (!ep) return IPC_ERR_INVALID;

    bool unlinked = false;
    spin_lock(&ep->lock);
    if (ipc_wait_queue_remove(&ep->senders, thread) ||
        ipc_wait_queue_remove(&ep->receivers, thread)) {
        if (ep->queued_msgs > 0U) {
            ep->queued_msgs--;
        }
        thread->ipc_wakeup_reason = (cancel_reason != 0) ? cancel_reason : IPC_ERR_CANCELLED;
        thread->ipc_deadline_ticks = 0U;
        unlinked = true;
    }
    ipc_endpoint_try_reclaim_locked(ep);
    spin_unlock(&ep->lock);

    if (unlinked) {
        sched_wakeup(thread);
        return IPC_OK;
    }
    return IPC_ERR_INVALID;
}

void ipc_endpoint_cancel_all_for_thread(bh_thread_t *thread, int cancel_reason) {
    if (!thread) return;
    for (uint32_t c = 0U; c < MAX_CPUS; ++c) {
        for (uint32_t s = 0U; s < BHARAT_IPC_ENDPOINTS_PER_CORE; ++s) {
            ipc_endpoint_t *ep = &g_endpoints[c][s];
            if (ep->in_use == 0U) continue;
            bool unlinked = false;
            spin_lock(&ep->lock);
            if (ep->in_use != 0U) {
                if (ipc_wait_queue_remove(&ep->senders, thread) ||
                    ipc_wait_queue_remove(&ep->receivers, thread)) {
                    if (ep->queued_msgs > 0U) {
                        ep->queued_msgs--;
                    }
                    thread->ipc_wakeup_reason = (cancel_reason != 0) ? cancel_reason : IPC_ERR_CANCELLED;
                    thread->ipc_deadline_ticks = 0U;
                    unlinked = true;
                }
                ipc_endpoint_try_reclaim_locked(ep);
            }
            spin_unlock(&ep->lock);
            if (unlinked) {
                sched_wakeup(thread);
            }
        }
    }
}

uint32_t ipc_endpoint_active_count_for_core(uint32_t core_id) {
    if (core_id >= MAX_CPUS) return 0U;
    uint32_t count = 0U;
    for (uint32_t i = 0U; i < BHARAT_IPC_ENDPOINTS_PER_CORE; ++i) {
        if (g_endpoints[core_id][i].in_use != 0U) {
            count++;
        }
    }
    return count;
}

int ipc_endpoint_create(capability_table_t* table, uint32_t* out_send_cap, uint32_t* out_recv_cap) {
    if (!table || !out_send_cap || !out_recv_cap) {
        return IPC_ERR_INVALID;
    }

    uint32_t owner_core = hal_cpu_get_id();
    if (owner_core >= MAX_CPUS || table->owner_core != owner_core) {
        return IPC_ERR_PERM;
    }

    for (uint32_t i = 0U; i < BHARAT_IPC_ENDPOINTS_PER_CORE; ++i) {
        ipc_endpoint_t *ep = &g_endpoints[owner_core][i];
        if (ep->in_use == 0U) {
            ep->in_use = 1U;
            ep->state = IPC_ENDPOINT_STATE_READY;
            ep->generation++;
            if (ep->generation == 0U) {
                ep->generation = 1U;
            }
            ep->owner_core = owner_core;
            ep->send_cap_refs = 0U;
            ep->recv_cap_refs = 0U;
            ep->active_claims = 0U;
            ep->queued_msgs = 0U;
            ep->dropped_msgs = 0U;
            ep->timeouts = 0U;
            ipc_endpoint_clear_msg_locked(ep);
            spin_lock_init(&ep->lock);
            sched_wait_queue_init(&ep->senders);
            sched_wait_queue_init(&ep->receivers);

            uint64_t obj_ref = endpoint_make_ref(owner_core, i, ep->generation);

            if (cap_table_grant(table, CAP_TYPE_ENDPOINT, obj_ref,
                                CAP_RIGHT_ENDPOINT_SEND | CAP_RIGHT_DELEGATE,
                                out_send_cap) != 0) {
                ep->in_use = 0U;
                ep->state = IPC_ENDPOINT_STATE_FREE;
                return IPC_ERR_NO_SPACE;
            }

            if (cap_table_grant(table, CAP_TYPE_ENDPOINT, obj_ref,
                                CAP_RIGHT_ENDPOINT_RECEIVE | CAP_RIGHT_DELEGATE,
                                out_recv_cap) != 0) {
                cap_table_revoke(table, *out_send_cap);
                ep->in_use = 0U;
                ep->state = IPC_ENDPOINT_STATE_FREE;
                return IPC_ERR_NO_SPACE;
            }

            return IPC_OK;
        }
    }

    return IPC_ERR_NO_SPACE;
}

int ipc_endpoint_send_ex(capability_table_t* table, uint32_t send_cap, const void* payload, uint32_t payload_len, uint64_t timeout_ticks, uint32_t cap_to_send, uint64_t cap_send_rights, const ipc_provenance_t* provenance) {
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

    uint32_t reply_cap_to_send = (provenance != NULL) ? provenance->reply_cap_to_send : 0U;
    uint64_t reply_cap_rights = (provenance != NULL) ? provenance->reply_cap_send_rights : 0U;

    // Slot 0: Primary / authority capability
    bh_cap_locator_t transfer_source = {0};
    cap_type_t transferred_type = CAP_TYPE_NONE;
    if (cap_to_send != 0U) {
        capability_entry_t sent_entry = {0};
        if (cap_table_lookup(table, cap_to_send, CAP_TYPE_NONE, cap_send_rights, &sent_entry) == 0) {
            transferred_type = sent_entry.type;
            if (cap_send_rights == 0U) {
                cap_send_rights = sent_entry.rights;
            }
        }
        if (bh_cap_transfer_capture(table, cap_to_send, cap_send_rights,
                                    &transfer_source) != K_OK) {
            return IPC_ERR_CAP_TRANSFER_NOT_ALLOWED;
        }
    }

    // Slot 1: Dedicated RPC reply send capability (must be CAP_TYPE_ENDPOINT with SEND only)
    bh_cap_locator_t reply_transfer_source = {0};
    if (reply_cap_to_send != 0U) {
        if (reply_cap_rights == 0U) {
            reply_cap_rights = CAP_RIGHT_ENDPOINT_SEND;
        }
        if ((reply_cap_rights & CAP_RIGHT_ENDPOINT_SEND) == 0U ||
            (reply_cap_rights & (CAP_RIGHT_ENDPOINT_RECEIVE | CAP_RIGHT_DELEGATE)) != 0U) {
            return IPC_ERR_CAP_TRANSFER_NOT_ALLOWED;
        }
        capability_entry_t reply_entry = {0};
        if (cap_table_lookup(table, reply_cap_to_send, CAP_TYPE_ENDPOINT,
                             CAP_RIGHT_ENDPOINT_SEND, &reply_entry) != 0) {
            return IPC_ERR_CAP_TRANSFER_NOT_ALLOWED;
        }
        if (bh_cap_transfer_capture(table, reply_cap_to_send, reply_cap_rights,
                                    &reply_transfer_source) != K_OK) {
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
        if (reply_cap_to_send != 0U &&
            bh_cap_transfer_capture(table, reply_cap_to_send, reply_cap_rights,
                                    &reply_transfer_source) != K_OK) {
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
            spin_lock(&ep->lock);
            bool was_queued = ipc_wait_queue_remove(&ep->senders, cur);
            if (cur->ipc_wakeup_reason == IPC_ERR_TIMEOUT) {
                ep->timeouts++;
                if (was_queued || ep->queued_msgs > 0U) {
                    ep->queued_msgs--;
                }
            }
            ipc_endpoint_try_reclaim_locked(ep);
            spin_unlock(&ep->lock);
            return cur->ipc_wakeup_reason;
        } else {
            spin_lock(&ep->lock);
            if (ep->queued_msgs > 0U) {
                ep->queued_msgs--;
            }
            spin_unlock(&ep->lock);
        }
    }

    const uint8_t* src = (const uint8_t*)payload;
    for (uint32_t i = 0; i < payload_len; ++i) {
        ep->msg.payload[i] = src[i];
    }
    ep->msg.msg_len = payload_len;

    // Store primary capability transfer metadata
    if (cap_to_send != 0U) {
        ep->msg.cap_transfer_id = cap_to_send;
        ep->msg.cap_transfer_rights = cap_send_rights;
        ep->cap_transfer_source = transfer_source;
    } else {
        ep->msg.cap_transfer_id = 0U;
        ep->msg.cap_transfer_rights = 0U;
        ep->cap_transfer_source = (bh_cap_locator_t){0};
    }

    // Store dedicated reply capability transfer metadata
    if (reply_cap_to_send != 0U) {
        ep->reply_cap_transfer_id = reply_cap_to_send;
        ep->reply_cap_transfer_rights = reply_cap_rights;
        ep->reply_cap_transfer_source = reply_transfer_source;
    } else {
        ep->reply_cap_transfer_id = 0U;
        ep->reply_cap_transfer_rights = 0U;
        ep->reply_cap_transfer_source = (bh_cap_locator_t){0};
    }

    // Capture and bind immutable kernel-owned provenance
    if (provenance != NULL && provenance->has_provenance) {
        ep->provenance = *provenance;
        ep->provenance.delivery_generation = ep->generation;
        ep->provenance.cap_type = transferred_type;
        ep->provenance.cap_rights = cap_send_rights;
        ep->provenance.has_provenance = 1U;
    } else {
        ep->provenance = (ipc_provenance_t){
            .delivery_generation = ep->generation,
            .cap_type = transferred_type,
            .cap_rights = cap_send_rights,
            .has_provenance = 0U,
        };
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

int ipc_endpoint_send(capability_table_t* table, uint32_t send_cap, const void* payload, uint32_t payload_len, uint64_t timeout_ticks, uint32_t cap_to_send, uint64_t cap_send_rights) {
    return ipc_endpoint_send_ex(table, send_cap, payload, payload_len, timeout_ticks, cap_to_send, cap_send_rights, NULL);
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
            spin_lock(&ep->lock);
            bool was_queued = ipc_wait_queue_remove(&ep->receivers, cur);
            if (cur->ipc_wakeup_reason == IPC_ERR_TIMEOUT) {
                ep->timeouts++;
                if (was_queued || ep->queued_msgs > 0U) {
                    ep->queued_msgs--;
                }
            }
            ipc_endpoint_try_reclaim_locked(ep);
            spin_unlock(&ep->lock);
            return cur->ipc_wakeup_reason;
        } else {
            spin_lock(&ep->lock);
            if (ep->queued_msgs > 0U) {
                ep->queued_msgs--;
            }
            spin_unlock(&ep->lock);
        }
    }

    if (out_payload_capacity < ep->msg.msg_len) {
        spin_unlock(&ep->lock);
        return IPC_ERR_NO_SPACE;
    }

    // Claim message and pin active_claims before dropping endpoint lock
    ep->has_msg = 2U;
    ep->active_claims++;
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

    spin_lock(&ep->lock);
    if (ep->active_claims > 0U) {
        ep->active_claims--;
    }

    if (ep->state != IPC_ENDPOINT_STATE_READY) {
        ipc_endpoint_clear_msg_locked(ep);
        ipc_endpoint_try_reclaim_locked(ep);
        spin_unlock(&ep->lock);
        if (newly_installed_cap_id != 0U) {
            cap_table_revoke(table, newly_installed_cap_id);
        }
        return IPC_ERR_CLOSED;
    }

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
        ipc_endpoint_clear_msg_locked(ep);
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

    ipc_endpoint_clear_msg_locked(ep);

    bh_thread_t* sender = sched_wait_queue_dequeue(&ep->senders);
    spin_unlock(&ep->lock);

    if (sender != NULL) {
        sender->ipc_wakeup_reason = IPC_OK;
        sender->ipc_deadline_ticks = 0;
        sched_wakeup(sender);
    }

    return IPC_OK;
}

int ipc_endpoint_receive_envelope(capability_table_t* table, uint32_t recv_cap, void* out_payload, uint32_t out_payload_capacity, uint32_t* out_received_len, uint64_t timeout_ticks, bh_ipc_receive_envelope_v1_t* out_envelope) {
    if (!table || !out_payload || out_payload_capacity == 0U || !out_received_len || !out_envelope) {
        return IPC_ERR_INVALID;
    }

    *out_received_len = 0U;
    for (size_t b = 0; b < sizeof(*out_envelope); ++b) {
        ((uint8_t*)out_envelope)[b] = 0U;
    }
    out_envelope->version = BH_IPC_RECEIVE_ENVELOPE_VERSION_V1;
    out_envelope->struct_size = (uint32_t)sizeof(bh_ipc_receive_envelope_v1_t);

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
            spin_lock(&ep->lock);
            bool was_queued = ipc_wait_queue_remove(&ep->receivers, cur);
            if (cur->ipc_wakeup_reason == IPC_ERR_TIMEOUT) {
                ep->timeouts++;
                if (was_queued || ep->queued_msgs > 0U) {
                    ep->queued_msgs--;
                }
            }
            ipc_endpoint_try_reclaim_locked(ep);
            spin_unlock(&ep->lock);
            return cur->ipc_wakeup_reason;
        } else {
            spin_lock(&ep->lock);
            if (ep->queued_msgs > 0U) {
                ep->queued_msgs--;
            }
            spin_unlock(&ep->lock);
        }
    }

    if (out_payload_capacity < ep->msg.msg_len) {
        spin_unlock(&ep->lock);
        return IPC_ERR_NO_SPACE;
    }

    // Claim message and pin active_claims before dropping endpoint lock
    ep->has_msg = 2U;
    ep->active_claims++;
    bh_cap_locator_t source = ep->cap_transfer_source;
    uint64_t transfer_rights = ep->msg.cap_transfer_rights;
    uint32_t transfer_id = ep->msg.cap_transfer_id;

    bh_cap_locator_t reply_source = ep->reply_cap_transfer_source;
    uint64_t reply_rights = ep->reply_cap_transfer_rights;
    uint32_t reply_id = ep->reply_cap_transfer_id;

    ipc_provenance_t prov = ep->provenance;
    spin_unlock(&ep->lock);

    uint32_t newly_installed_cap_id = 0U;
    uint32_t newly_installed_reply_cap_id = 0U;
    kstatus_t install_ret = K_OK;

    if (transfer_id != 0U) {
        install_ret = bh_cap_transfer_install(&source, table, transfer_rights,
                                              &newly_installed_cap_id);
    }

    if (install_ret == K_OK && reply_id != 0U) {
        install_ret = bh_cap_transfer_install(&reply_source, table, reply_rights,
                                              &newly_installed_reply_cap_id);
        if (install_ret != K_OK && newly_installed_cap_id != 0U) {
            // Atomic rollback of Slot 0 if Slot 1 installation fails (Amendment D)
            cap_table_revoke(table, newly_installed_cap_id);
            newly_installed_cap_id = 0U;
        }
    }

    spin_lock(&ep->lock);
    if (ep->active_claims > 0U) {
        ep->active_claims--;
    }

    if (ep->state != IPC_ENDPOINT_STATE_READY) {
        ipc_endpoint_clear_msg_locked(ep);
        ipc_endpoint_try_reclaim_locked(ep);
        spin_unlock(&ep->lock);
        if (newly_installed_cap_id != 0U) {
            cap_table_revoke(table, newly_installed_cap_id);
        }
        if (newly_installed_reply_cap_id != 0U) {
            cap_table_revoke(table, newly_installed_reply_cap_id);
        }
        return IPC_ERR_CLOSED;
    }

    if (install_ret == K_ERR_NO_RESOURCES) {
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
        ipc_endpoint_clear_msg_locked(ep);
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

    // Fill authenticated envelope
    out_envelope->version = BH_IPC_RECEIVE_ENVELOPE_VERSION_V1;
    out_envelope->struct_size = (uint32_t)sizeof(bh_ipc_receive_envelope_v1_t);
    out_envelope->valid_fields = 0U;

    if (prov.sender_pid != 0U) {
        out_envelope->sender_pid = prov.sender_pid;
        out_envelope->valid_fields |= BH_IPC_ENVELOPE_F_SENDER_PID;
    }
    if (prov.sender_process_object_id != 0U) {
        out_envelope->sender_process_object_id = prov.sender_process_object_id;
        out_envelope->valid_fields |= BH_IPC_ENVELOPE_F_SENDER_PROCESS_OBJECT;
    }
    if (prov.sender_cspace_id != 0U) {
        out_envelope->sender_cspace_id = prov.sender_cspace_id;
        out_envelope->valid_fields |= BH_IPC_ENVELOPE_F_SENDER_CSPACE;
    }
    out_envelope->sender_owner_core = prov.sender_owner_core;
    out_envelope->valid_fields |= BH_IPC_ENVELOPE_F_SENDER_OWNER_CORE;

    out_envelope->delivery_generation = prov.delivery_generation;
    out_envelope->valid_fields |= BH_IPC_ENVELOPE_F_DELIVERY_GENERATION;

    if (prov.boot_session_id != 0U) {
        out_envelope->boot_session_id = prov.boot_session_id;
        out_envelope->valid_fields |= BH_IPC_ENVELOPE_F_BOOT_SESSION;
    } else {
        out_envelope->boot_session_id = 0U;
    }

    if (prov.service_incarnation_id != 0U) {
        out_envelope->service_incarnation_id = prov.service_incarnation_id;
        out_envelope->valid_fields |= BH_IPC_ENVELOPE_F_SERVICE_INCARNATION;
    } else {
        out_envelope->service_incarnation_id = 0U;
    }

    if (reply_id != 0U) {
        /* Explicit 2-slot mode: Slot 0 is authority_cap, Slot 1 is reply_send_cap */
        out_envelope->reply_send_cap = newly_installed_reply_cap_id;
        if (newly_installed_reply_cap_id != 0U) {
            out_envelope->valid_fields |= BH_IPC_ENVELOPE_F_REPLY_SEND_CAP;
        }
        if (newly_installed_cap_id != 0U) {
            out_envelope->authority_cap = newly_installed_cap_id;
            out_envelope->authority_object_type = (uint32_t)prov.cap_type;
            out_envelope->authority_rights = transfer_rights;
            out_envelope->valid_fields |= BH_IPC_ENVELOPE_F_AUTHORITY_CAP;
        } else {
            out_envelope->authority_cap = 0U;
            out_envelope->authority_object_type = 0U;
            out_envelope->authority_rights = 0U;
        }
    } else if (newly_installed_cap_id != 0U) {
        /* Single-slot mode: preserve existing classification */
        if (prov.cap_type == CAP_TYPE_ENDPOINT &&
            (prov.cap_rights & CAP_RIGHT_ENDPOINT_SEND) != 0U &&
            (prov.cap_rights & CAP_RIGHT_ENDPOINT_RECEIVE) == 0U) {
            out_envelope->reply_send_cap = newly_installed_cap_id;
            out_envelope->authority_cap = 0U;
            out_envelope->authority_object_type = 0U;
            out_envelope->authority_rights = 0U;
            out_envelope->valid_fields |= BH_IPC_ENVELOPE_F_REPLY_SEND_CAP;
        } else {
            out_envelope->reply_send_cap = 0U;
            out_envelope->authority_cap = newly_installed_cap_id;
            out_envelope->authority_object_type = (uint32_t)prov.cap_type;
            out_envelope->authority_rights = transfer_rights;
            out_envelope->valid_fields |= BH_IPC_ENVELOPE_F_AUTHORITY_CAP;
        }
    } else {
        out_envelope->authority_cap = 0U;
        out_envelope->reply_send_cap = 0U;
        out_envelope->authority_object_type = 0U;
        out_envelope->authority_rights = 0U;
    }
    out_envelope->reserved0 = 0U;
    out_envelope->message_id = 0U;

    ipc_endpoint_clear_msg_locked(ep);

    bh_thread_t* sender = sched_wait_queue_dequeue(&ep->senders);
    spin_unlock(&ep->lock);

    if (sender != NULL) {
        sender->ipc_wakeup_reason = IPC_OK;
        sender->ipc_deadline_ticks = 0;
        sched_wakeup(sender);
    }

    return IPC_OK;
}
