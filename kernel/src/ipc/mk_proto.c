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
}
