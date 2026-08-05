#include "../../include/ipc/mk_proto.h"
#include "../../include/ipc/mk_dispatch.h"
#include <hal/hal.h>
#include <bharat/cpu_local.h>

#include "fabric/mk_mpsc_ring.c"
#include "fabric/mk_endpoint.c"
#include "fabric/mk_transaction.c"
#include "fabric/mk_replay.c"
#include "fabric/mk_fabric.c"

int mk_proto_txn_table_init(void) {
    bh_mk_fabric_init(BHARAT_MAX_CPUS);
    return 0;
}

int mk_proto_txn_begin(uint64_t txn_id, uint32_t remote_core, uint32_t msg_type, uint64_t deadline_ticks) {
    bh_mk_tx_handle_t handle;
    kstatus_t status = bh_mk_tx_alloc(remote_core, (remote_core << 24) | BH_MK_ENDPOINT_LEGACY, 0, msg_type, deadline_ticks, &handle);
    if (status != K_OK) {
        return -1;
    }

    uint32_t core_id = hal_cpu_get_id();
    bh_mk_core_fabric_t *f = bh_mk_get_core_fabric(core_id);
    if (f) {
        f->txns.entries[handle.slot].legacy_txn_id = txn_id;
        f->txns.entries[handle.slot].state = BH_MK_TXN_SENT;
    }
    return 0;
}

kstatus_t bh_mk_legacy_tx_complete_by_id(uint64_t legacy_txn_id, kstatus_t result) {
    uint32_t core_id = hal_cpu_get_id();
    bh_mk_core_fabric_t *f = bh_mk_get_core_fabric(core_id);
    if (!f) return K_ERR_NOT_FOUND;

    for (uint32_t i = 0; i < BH_MK_TX_TABLE_SIZE; i++) {
        bh_mk_tx_entry_t *entry = &f->txns.entries[i];
        if (atomic_load_explicit(&entry->in_use, memory_order_relaxed) && entry->legacy_txn_id == legacy_txn_id) {
            bh_mk_tx_handle_t handle = { .slot = i, .generation = entry->generation };
            bh_mk_tx_complete(handle, entry->dst_core, entry->dst_endpoint, result);
            kstatus_t dummy;
            bh_mk_tx_reap(handle, &dummy);
            return K_OK;
        }
    }
    return K_ERR_NOT_FOUND;
}

int mk_proto_txn_complete(uint64_t txn_id, int result) {
    return bh_mk_legacy_tx_complete_by_id(txn_id, result == 0 ? K_OK : K_ERR_CANCELLED) == K_OK ? 0 : -1;
}

int mk_proto_txn_poll_timeouts(uint64_t now_ticks) {
    uint32_t core_id = hal_cpu_get_id();
    bh_mk_core_fabric_t *f = bh_mk_get_core_fabric(core_id);
    if (!f) return -1;

    int timed_out_count = 0;
    for (uint32_t i = 0; i < BH_MK_TX_TABLE_SIZE; i++) {
        bh_mk_tx_entry_t *entry = &f->txns.entries[i];
        if (atomic_load_explicit(&entry->in_use, memory_order_relaxed) && entry->state == BH_MK_TXN_SENT) {
            if (now_ticks >= entry->deadline_ticks) {
                entry->state = BH_MK_TXN_TIMED_OUT;
                entry->result = K_ERR_TIMEOUT;
                atomic_store_explicit(&entry->in_use, 0, memory_order_release);
                entry->generation++;
                timed_out_count++;
            }
        }
    }
    return timed_out_count;
}

int mk_proto_txn_lookup(uint64_t txn_id, mk_proto_txn_entry_t *out_entry) {
    if (!out_entry) return -1;

    uint32_t core_id = hal_cpu_get_id();
    bh_mk_core_fabric_t *f = bh_mk_get_core_fabric(core_id);
    if (!f) return -1;

    for (uint32_t i = 0; i < BH_MK_TX_TABLE_SIZE; i++) {
        bh_mk_tx_entry_t *entry = &f->txns.entries[i];
        if (atomic_load_explicit(&entry->in_use, memory_order_relaxed) && entry->legacy_txn_id == txn_id) {
            out_entry->txn_id = txn_id;
            out_entry->remote_core = entry->dst_core;
            out_entry->msg_type = entry->opcode;
            if (entry->state == BH_MK_TXN_ACKED) {
                out_entry->state = MK_TXN_STATE_ACKED;
            } else if (entry->state == BH_MK_TXN_TIMED_OUT) {
                out_entry->state = MK_TXN_STATE_TIMED_OUT;
            } else {
                out_entry->state = MK_TXN_STATE_SENT;
            }
            out_entry->deadline_ticks = entry->deadline_ticks;
            out_entry->retry_count = 0;
            out_entry->completion_status = entry->result == K_OK ? 0 : -1;
            out_entry->in_use = 1;
            return 0;
        }
    }
    return -1;
}

int mk_proto_send_tracked(mk_channel_t *channel, uint32_t msg_type,
                          void *payload, uint32_t size,
                          uint64_t txn_id, uint64_t deadline_ticks) {
    if (!channel) return -1;

    int ack_required = 1;
    if (msg_type == MK_MSG_TYPE_ACK || msg_type == MK_MSG_TYPE_NACK ||
        msg_type == MK_MSG_THREAD_HANDOFF_ACK || msg_type == MK_MSG_THREAD_HANDOFF_NACK ||
        msg_type == MK_MSG_THREAD_LOOKUP_RESP) {
        ack_required = 0;
    }

    if (ack_required) {
        if (mk_proto_txn_begin(txn_id, channel->dst_core, msg_type, deadline_ticks) != 0) {
            return -1;
        }
    }

    int ret = mk_send_message(channel, msg_type, payload, size);

    if (ret != 0 && ack_required) {
        mk_proto_txn_complete(txn_id, -1);
    }

    return ret;
}

static uint32_t mk_proto_policy_flags_for(uint32_t msg_type) {
    switch (msg_type) {
        case MK_MSG_THREAD_LOOKUP_REQ:
        case MK_MSG_PROCESS_LOOKUP_REQ:
        case MK_MSG_CAP_LOOKUP_REQ:
            return MK_PROTO_POLICY_ACK_REQUIRED |
                   MK_PROTO_POLICY_IDEMPOTENT |
                   MK_PROTO_POLICY_RETRYABLE;

        case MK_MSG_THREAD_HANDOFF_REQ:
        case MK_MSG_FRAME_ALLOC_REQ:
        case MK_MSG_FRAME_FREE_REQ:
        case MK_MSG_FRAME_MAP_REQ:
        case MK_MSG_FRAME_UNMAP_REQ:
        case MK_MSG_CAP_GRANT_REQ:
        case MK_MSG_CAP_REVOKE_REQ:
            return MK_PROTO_POLICY_ACK_REQUIRED |
                   MK_PROTO_POLICY_STATE_MUTATION;

        default:
            return MK_PROTO_POLICY_ACK_REQUIRED;
    }
}

int mk_proto_get_policy(uint32_t msg_type, mk_proto_policy_t *out_policy) {
    if (!out_policy) {
        return -1;
    }
    out_policy->msg_type = msg_type;
    out_policy->flags = mk_proto_policy_flags_for(msg_type);
    out_policy->max_retries = (out_policy->flags & MK_PROTO_POLICY_RETRYABLE) ? 2U : 0U;
    return 0;
}

int mk_proto_is_idempotent(uint32_t msg_type) {
    return (mk_proto_policy_flags_for(msg_type) & MK_PROTO_POLICY_IDEMPOTENT) != 0U;
}

int mk_proto_should_retry(uint32_t msg_type, mk_proto_result_t result,
                          uint32_t retry_count) {
    uint32_t flags = mk_proto_policy_flags_for(msg_type);

    if (!(flags & MK_PROTO_POLICY_RETRYABLE)) {
        return 0;
    }

    if (result == MK_PROTO_RESULT_TIMEOUT || result == MK_PROTO_RESULT_DUPLICATE) {
        if (retry_count < 2U) {
            return 1;
        }
    }

    return 0;
}

mk_txn_state_t mk_proto_result_to_txn_state(mk_proto_result_t result) {
    switch (result) {
        case MK_PROTO_RESULT_OK:
            return MK_TXN_STATE_ACKED;
        case MK_PROTO_RESULT_TIMEOUT:
            return MK_TXN_STATE_TIMED_OUT;
        case MK_PROTO_RESULT_STALE_ENDPOINT:
        case MK_PROTO_RESULT_CAP_REVOKED:
        case MK_PROTO_RESULT_BAD_AUTH:
        case MK_PROTO_RESULT_BAD_ROUTE:
        case MK_PROTO_RESULT_BAD_PAYLOAD:
        case MK_PROTO_RESULT_UNSUPPORTED:
        case MK_PROTO_RESULT_RETRY_NOT_ALLOWED:
            return MK_TXN_STATE_CANCELLED;
        case MK_PROTO_RESULT_DUPLICATE:
            return MK_TXN_STATE_REPLIED;
        default:
            return MK_TXN_STATE_CANCELLED;
    }
}

uint32_t mk_proto_result_to_reason_code(mk_proto_result_t result) {
    switch (result) {
        case MK_PROTO_RESULT_OK: return MK_REASON_SUCCESS;
        case MK_PROTO_RESULT_STALE_ENDPOINT: return MK_REASON_STALE_ENDPOINT;
        case MK_PROTO_RESULT_CAP_REVOKED: return MK_REASON_CAP_REVOKED;
        case MK_PROTO_RESULT_DUPLICATE: return MK_REASON_DUPLICATE;
        case MK_PROTO_RESULT_TIMEOUT: return MK_REASON_TIMEOUT;
        case MK_PROTO_RESULT_BAD_AUTH: return MK_REASON_BAD_AUTH;
        case MK_PROTO_RESULT_BAD_ROUTE: return MK_REASON_BAD_ROUTE;
        case MK_PROTO_RESULT_BAD_PAYLOAD: return MK_REASON_BAD_PAYLOAD;
        case MK_PROTO_RESULT_UNSUPPORTED: return MK_REASON_UNSUPPORTED;
        case MK_PROTO_RESULT_RETRY_NOT_ALLOWED: return MK_REASON_RETRY_NOT_ALLOWED;
        default: return MK_REASON_UNSUPPORTED;
    }
}
