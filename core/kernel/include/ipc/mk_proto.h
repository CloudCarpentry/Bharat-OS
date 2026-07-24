#ifndef BHARAT_MK_PROTO_H
#define BHARAT_MK_PROTO_H

#include "../core/multikernel.h"

// Delivery Flags
#define MK_MSG_FLAG_ACK_REQUIRED  (1U << 0)
#define MK_MSG_FLAG_IS_REPLY      (1U << 1)
#define MK_MSG_FLAG_ERROR         (1U << 2)

// Reason Codes / Ack Nack
#define MK_REASON_SUCCESS              0U
#define MK_REASON_BAD_AUTH             1U
#define MK_REASON_UNSUPPORTED          2U
#define MK_REASON_TIMEOUT              3U
#define MK_REASON_STALE_ENDPOINT       4U
#define MK_REASON_CAP_REVOKED          5U
#define MK_REASON_DUPLICATE            6U
#define MK_REASON_BAD_ROUTE            7U
#define MK_REASON_BAD_PAYLOAD          8U
#define MK_REASON_RETRY_NOT_ALLOWED    9U

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

int mk_proto_send_tracked(mk_channel_t *channel, uint32_t msg_type,
                          void *payload, uint32_t size,
                          uint64_t txn_id, uint64_t deadline_ticks);
// Conservative policy flags used by the L1 protocol layer.
typedef enum {
    MK_PROTO_POLICY_NONE            = 0,
    MK_PROTO_POLICY_ACK_REQUIRED    = 1U << 0,
    MK_PROTO_POLICY_IDEMPOTENT      = 1U << 1,
    MK_PROTO_POLICY_RETRYABLE       = 1U << 2,
    MK_PROTO_POLICY_STATE_MUTATION  = 1U << 3,
} mk_proto_policy_flags_t;

typedef enum {
    MK_PROTO_RESULT_OK = 0,
    MK_PROTO_RESULT_STALE_ENDPOINT,
    MK_PROTO_RESULT_CAP_REVOKED,
    MK_PROTO_RESULT_DUPLICATE,
    MK_PROTO_RESULT_TIMEOUT,
    MK_PROTO_RESULT_BAD_AUTH,
    MK_PROTO_RESULT_BAD_ROUTE,
    MK_PROTO_RESULT_BAD_PAYLOAD,
    MK_PROTO_RESULT_UNSUPPORTED,
    MK_PROTO_RESULT_RETRY_NOT_ALLOWED,
} mk_proto_result_t;

typedef struct {
    uint32_t msg_type;
    uint32_t flags;
    uint32_t max_retries;
} mk_proto_policy_t;

int mk_proto_get_policy(uint32_t msg_type, mk_proto_policy_t *out_policy);
int mk_proto_is_idempotent(uint32_t msg_type);
int mk_proto_should_retry(uint32_t msg_type, mk_proto_result_t result,
                          uint32_t retry_count);
mk_txn_state_t mk_proto_result_to_txn_state(mk_proto_result_t result);
uint32_t mk_proto_result_to_reason_code(mk_proto_result_t result);

#endif // BHARAT_MK_PROTO_H
