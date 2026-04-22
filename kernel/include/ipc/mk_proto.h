#ifndef BHARAT_MK_PROTO_H
#define BHARAT_MK_PROTO_H

#include "../core/multikernel.h"

// Delivery Flags
#define MK_MSG_FLAG_ACK_REQUIRED  (1U << 0)
#define MK_MSG_FLAG_IS_REPLY      (1U << 1)
#define MK_MSG_FLAG_ERROR         (1U << 2)

// Reason Codes / Ack Nack
#define MK_REASON_SUCCESS      0
#define MK_REASON_BAD_AUTH     1
#define MK_REASON_UNSUPPORTED  2
#define MK_REASON_TIMEOUT      3

// Distributed Message Types
#define MK_MSG_MEM_RESERVE      10U
#define MK_MSG_PROC_LOOKUP      11U
#define MK_MSG_CAP_RETYPE       12U
#define MK_MSG_TLB_SHOOTDOWN    13U




typedef struct {
    uint64_t txn_id;
    uint32_t remote_core;
    uint32_t msg_type;
    mk_txn_state_t state;
    uint64_t deadline_ticks;
    uint32_t retry_count;
    int completion_status;
    uint8_t in_use;
} mk_proto_txn_entry_t;

int mk_proto_txn_table_init(void);
int mk_proto_txn_begin(uint64_t txn_id, uint32_t remote_core, uint32_t msg_type, uint64_t deadline_ticks);
int mk_proto_txn_complete(uint64_t txn_id, int result);
int mk_proto_txn_poll_timeouts(uint64_t now_ticks);
int mk_proto_txn_lookup(uint64_t txn_id, mk_proto_txn_entry_t *out_entry);

int mk_proto_is_idempotent(uint32_t msg_type);
int mk_proto_should_retry(uint32_t msg_type, uint32_t retry_count);


int mk_proto_send_tracked(mk_channel_t *channel, uint32_t msg_type,
                          void *payload, uint32_t size,
                          uint64_t txn_id, uint64_t deadline_ticks);
#endif // BHARAT_MK_PROTO_H
