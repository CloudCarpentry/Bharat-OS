#include "../../include/ipc/mk_proto.h"
#include "../../include/ipc/mk_dispatch.h"
#include <hal/hal.h>
#include <bharat/cpu_local.h>

#define MK_PROTO_MAX_TXNS 256

typedef struct {
    mk_proto_txn_entry_t entries[MK_PROTO_MAX_TXNS];
} mk_proto_txn_table_t;

// Per-core transaction table
static mk_proto_txn_table_t g_txn_tables[32]; // Fixed size, assume up to 32 cores

int mk_proto_txn_table_init(void) {
    uint32_t core_id = hal_cpu_get_id();
    if (core_id >= 32) return -1;

    mk_proto_txn_table_t *table = &g_txn_tables[core_id];
    for (int i = 0; i < MK_PROTO_MAX_TXNS; i++) {
        table->entries[i].in_use = 0;
        table->entries[i].state = MK_TXN_STATE_FREE;
    }
    return 0;
}

int mk_proto_txn_begin(uint64_t txn_id, uint32_t remote_core, uint32_t msg_type, uint64_t deadline_ticks) {
    uint32_t core_id = hal_cpu_get_id();
    if (core_id >= 32) return -1;

    mk_proto_txn_table_t *table = &g_txn_tables[core_id];

    // Check for duplicate live txn
    for (int i = 0; i < MK_PROTO_MAX_TXNS; i++) {
        if (table->entries[i].in_use && table->entries[i].txn_id == txn_id) {
            return -1; // Duplicate live txn
        }
    }

    // Find free slot
    for (int i = 0; i < MK_PROTO_MAX_TXNS; i++) {
        if (!table->entries[i].in_use) {
            table->entries[i].txn_id = txn_id;
            table->entries[i].remote_core = remote_core;
            table->entries[i].msg_type = msg_type;
            table->entries[i].state = MK_TXN_STATE_SENT;
            table->entries[i].deadline_ticks = deadline_ticks;
            table->entries[i].retry_count = 0;
            table->entries[i].completion_status = 0;
            table->entries[i].in_use = 1;
            return 0;
        }
    }

    return -1; // Table full
}

int mk_proto_txn_complete(uint64_t txn_id, int result) {
    uint32_t core_id = hal_cpu_get_id();
    if (core_id >= 32) return -1;

    mk_proto_txn_table_t *table = &g_txn_tables[core_id];

    for (int i = 0; i < MK_PROTO_MAX_TXNS; i++) {
        if (table->entries[i].in_use && table->entries[i].txn_id == txn_id) {
            table->entries[i].state = MK_TXN_STATE_ACKED;
            table->entries[i].completion_status = result;
            // Depending on design, we could clear in_use here or wait for caller to reap
            table->entries[i].in_use = 0;
            return 0;
        }
    }

    return -1; // Not found
}

int mk_proto_txn_poll_timeouts(uint64_t now_ticks) {
    uint32_t core_id = hal_cpu_get_id();
    if (core_id >= 32) return -1;

    mk_proto_txn_table_t *table = &g_txn_tables[core_id];
    int timed_out_count = 0;

    for (int i = 0; i < MK_PROTO_MAX_TXNS; i++) {
        if (table->entries[i].in_use && table->entries[i].state == MK_TXN_STATE_SENT) {
            if (now_ticks >= table->entries[i].deadline_ticks) {
                table->entries[i].state = MK_TXN_STATE_TIMED_OUT;
                table->entries[i].completion_status = MK_REASON_TIMEOUT;
                table->entries[i].in_use = 0;
                timed_out_count++;
            }
        }
    }

    return timed_out_count;
}

int mk_proto_txn_lookup(uint64_t txn_id, mk_proto_txn_entry_t *out_entry) {
    if (!out_entry) return -1;

    uint32_t core_id = hal_cpu_get_id();
    if (core_id >= 32) return -1;

    mk_proto_txn_table_t *table = &g_txn_tables[core_id];

    for (int i = 0; i < MK_PROTO_MAX_TXNS; i++) {
        if (table->entries[i].in_use && table->entries[i].txn_id == txn_id) {
            *out_entry = table->entries[i];
            return 0;
        }
    }

    return -1;
}

int mk_proto_is_idempotent(uint32_t msg_type) {
    switch (msg_type) {
        case MK_MSG_PROC_LOOKUP:
        case MK_MSG_THREAD_LOOKUP_REQ:
            return 1;
        default:
            return 0;
    }
}

int mk_proto_should_retry(uint32_t msg_type, uint32_t retry_count) {
    if (retry_count >= 3) {
        return 0;
    }

    if (mk_proto_is_idempotent(msg_type)) {
        return 1;
    }

    return 0; // State-changing ops do not auto-retry
}

int mk_proto_send_tracked(mk_channel_t *channel, uint32_t msg_type,
                          void *payload, uint32_t size,
                          uint64_t txn_id, uint64_t deadline_ticks) {
    if (!channel) return -1;

    // Determine if ACK is required (basic heuristic for now)
    int ack_required = 1;
    if (msg_type == MK_MSG_TYPE_ACK || msg_type == MK_MSG_TYPE_NACK ||
        msg_type == MK_MSG_THREAD_HANDOFF_ACK || msg_type == MK_MSG_THREAD_HANDOFF_NACK ||
        msg_type == MK_MSG_THREAD_LOOKUP_RESP) {
        ack_required = 0;
    }

    if (ack_required) {
        if (mk_proto_txn_begin(txn_id, channel->dst_core, msg_type, deadline_ticks) != 0) {
            return -1; // Failed to begin tracked txn
        }
    }

    int ret = mk_send_message(channel, msg_type, payload, size);

    if (ret != 0 && ack_required) {
        // Rollback txn on immediate send failure
        mk_proto_txn_complete(txn_id, MK_REASON_BAD_AUTH); // Arbitrary failure code for rollback
    }

    return ret;

// Minimal, non-invasive L1 policy helpers. These do not change runtime
// behavior yet; they provide a single place to encode retry/idempotency
// rules and map outcomes to transaction states.

static uint32_t mk_proto_policy_flags_for(uint32_t msg_type) {
    switch (msg_type) {
        // Read-only / lookup style operations → idempotent + retryable
        case MK_MSG_THREAD_LOOKUP_REQ:
        case MK_MSG_PROCESS_LOOKUP_REQ:
        case MK_MSG_CAP_LOOKUP_REQ:
            return MK_PROTO_POLICY_ACK_REQUIRED |
                   MK_PROTO_POLICY_IDEMPOTENT |
                   MK_PROTO_POLICY_RETRYABLE;

        // State-changing operations → no automatic retry
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
            // Conservative default
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
        // Retry bounded times only
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
