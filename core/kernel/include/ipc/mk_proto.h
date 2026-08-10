#ifndef BHARAT_MK_PROTO_H
#define BHARAT_MK_PROTO_H

#include "../core/multikernel.h"
#include "kernel/status.h"
#include "hal/hal_timer.h"
#include <stdatomic.h>

// BHARAT_MAX_CPUS fallback
#ifndef BHARAT_MAX_CPUS
#define BHARAT_MAX_CPUS 256U
#endif

// Status mapping fallbacks
#ifndef K_ERR_WOULD_BLOCK
#define K_ERR_WOULD_BLOCK K_ERR_BUSY
#endif
#ifndef K_ERR_STALE
#define K_ERR_STALE K_ERR_CAP_STALE
#endif
#ifndef K_ERR_VERSION
#define K_ERR_VERSION K_ERR_UNSUPPORTED
#endif

// BHARAT-OS Reliable Multikernel Message Fabric Definitions
#define BH_MK_ABI_VERSION_V1 1U
#define BH_MK_INLINE_PAYLOAD_MAX 128U

// Profile-based ingress queue capacity selection
#if defined(BHARAT_PROFILE_MMU_FULL)
# define BH_MK_LANE_CONTROL_CAP 64U
# define BH_MK_LANE_NORMAL_CAP  128U
# define BH_MK_LANE_BULK_CAP    32U
#elif defined(BHARAT_PROFILE_MMU_LITE)
# define BH_MK_LANE_CONTROL_CAP 32U
# define BH_MK_LANE_NORMAL_CAP  64U
# define BH_MK_LANE_BULK_CAP    16U
#else /* MPU / RT / default */
# define BH_MK_LANE_CONTROL_CAP 32U
# define BH_MK_LANE_NORMAL_CAP  32U
# define BH_MK_LANE_BULK_CAP    8U
#endif

// Delivery/Legacy Flags
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
    uint16_t abi_version;
    uint16_t header_size;

    uint16_t message_class;
    uint16_t opcode;

    uint32_t flags;
    uint32_t payload_size;

    uint32_t src_core;
    uint32_t dst_core;

    uint64_t src_endpoint;
    uint32_t src_endpoint_generation;
    uint32_t pad1;

    uint64_t dst_endpoint;
    uint32_t dst_endpoint_generation;
    uint32_t pad2;

    uint32_t txn_slot;
    uint32_t txn_generation;

    uint64_t sequence;
    uint64_t deadline_ticks;

    uint32_t auth_kind;
    uint32_t auth_length;

    uint32_t checksum;
    uint32_t reserved;
} bh_mk_wire_header_v1_t;

_Static_assert(sizeof(bh_mk_wire_header_v1_t) == 96, "bh_mk_wire_header_v1_t size must be exactly 96 bytes");
_Static_assert(__builtin_offsetof(bh_mk_wire_header_v1_t, abi_version) == 0, "abi_version offset mismatch");
_Static_assert(__builtin_offsetof(bh_mk_wire_header_v1_t, header_size) == 2, "header_size offset mismatch");
_Static_assert(__builtin_offsetof(bh_mk_wire_header_v1_t, message_class) == 4, "message_class offset mismatch");
_Static_assert(__builtin_offsetof(bh_mk_wire_header_v1_t, opcode) == 6, "opcode offset mismatch");
_Static_assert(__builtin_offsetof(bh_mk_wire_header_v1_t, flags) == 8, "flags offset mismatch");
_Static_assert(__builtin_offsetof(bh_mk_wire_header_v1_t, payload_size) == 12, "payload_size offset mismatch");
_Static_assert(__builtin_offsetof(bh_mk_wire_header_v1_t, src_core) == 16, "src_core offset mismatch");
_Static_assert(__builtin_offsetof(bh_mk_wire_header_v1_t, dst_core) == 20, "dst_core offset mismatch");
_Static_assert(__builtin_offsetof(bh_mk_wire_header_v1_t, src_endpoint) == 24, "src_endpoint offset mismatch");
_Static_assert(__builtin_offsetof(bh_mk_wire_header_v1_t, src_endpoint_generation) == 32, "src_endpoint_generation offset mismatch");
_Static_assert(__builtin_offsetof(bh_mk_wire_header_v1_t, dst_endpoint) == 40, "dst_endpoint offset mismatch");
_Static_assert(__builtin_offsetof(bh_mk_wire_header_v1_t, dst_endpoint_generation) == 48, "dst_endpoint_generation offset mismatch");
_Static_assert(__builtin_offsetof(bh_mk_wire_header_v1_t, txn_slot) == 56, "txn_slot offset mismatch");
_Static_assert(__builtin_offsetof(bh_mk_wire_header_v1_t, txn_generation) == 60, "txn_generation offset mismatch");
_Static_assert(__builtin_offsetof(bh_mk_wire_header_v1_t, sequence) == 64, "sequence offset mismatch");
_Static_assert(__builtin_offsetof(bh_mk_wire_header_v1_t, deadline_ticks) == 72, "deadline_ticks offset mismatch");
_Static_assert(__builtin_offsetof(bh_mk_wire_header_v1_t, auth_kind) == 80, "auth_kind offset mismatch");
_Static_assert(__builtin_offsetof(bh_mk_wire_header_v1_t, auth_length) == 84, "auth_length offset mismatch");
_Static_assert(__builtin_offsetof(bh_mk_wire_header_v1_t, checksum) == 88, "checksum offset mismatch");
_Static_assert(__builtin_offsetof(bh_mk_wire_header_v1_t, reserved) == 92, "reserved offset mismatch");

typedef struct {
    bh_mk_wire_header_v1_t header;
    uint8_t payload[BH_MK_INLINE_PAYLOAD_MAX];
} bh_mk_wire_message_t;

typedef struct __attribute__((aligned(64))) {
    _Atomic uint64_t sequence;
    bh_mk_wire_message_t message;
} bh_mk_ring_slot_t;

typedef struct {
    _Atomic uint64_t producer_head;
    uint64_t consumer_tail;

    _Atomic uint32_t available_credits;
    uint32_t capacity;
    uint32_t mask;

    bh_mk_ring_slot_t *slots;
} bh_mk_mpsc_ring_t;

typedef struct {
    uint32_t slot;
    uint32_t generation;
} bh_mk_tx_handle_t;

typedef enum {
    BH_MK_TXN_FREE = 0,
    BH_MK_TXN_RESERVED,
    BH_MK_TXN_PUBLISHED,
    BH_MK_TXN_SENT,
    BH_MK_TXN_WAITING,
    BH_MK_TXN_ACKED,
    BH_MK_TXN_NACKED,
    BH_MK_TXN_TIMED_OUT,
    BH_MK_TXN_CANCELLED,
    BH_MK_TXN_REAPED
} bh_mk_txn_state_t;

typedef struct {
    uint32_t generation;
    bh_mk_txn_state_t state;
    uint64_t deadline_ticks;
    kstatus_t result;
    uint32_t dst_core;
    uint64_t dst_endpoint;
    uint32_t msg_class;
    uint32_t opcode;
    uint64_t legacy_txn_id;
    _Atomic uint8_t in_use;
} bh_mk_tx_entry_t;

#define BH_MK_TX_TABLE_SIZE 128U

typedef struct {
    bh_mk_tx_entry_t entries[BH_MK_TX_TABLE_SIZE];
} bh_mk_tx_table_t;

#define BH_MK_REPLAY_CACHE_SIZE 64U

typedef struct {
    uint32_t src_core;
    uint64_t src_endpoint;
    uint32_t src_endpoint_gen;
    uint32_t txn_slot;
    uint32_t txn_gen;
    uint16_t msg_class;
    uint16_t opcode;
    uint64_t sequence;
    uint64_t timestamp;
} bh_mk_replay_entry_t;

typedef struct {
    bh_mk_replay_entry_t entries[BH_MK_REPLAY_CACHE_SIZE];
    uint32_t head;
} bh_mk_replay_cache_t;

#define BH_MK_MAX_ENDPOINTS 64U
typedef uint64_t bh_mk_endpoint_handle_t;
#define BH_MK_ENDPOINT_INVALID 0xFFFFFFFFFFFFFFFFULL
#define BH_MK_ENDPOINT_LEGACY  0xFFFFFFFFFFFFFFFEULL

typedef kstatus_t (*bh_mk_handler_t)(
    bh_mk_endpoint_handle_t source_endpoint,
    const bh_mk_wire_message_t *message,
    void *ctx);

typedef struct {
    bh_mk_handler_t handler_fn;
    void *ctx;
    uint32_t message_class;
    uint32_t opcode;
    uint32_t generation;
    uint8_t bound;
} bh_mk_endpoint_entry_t;

typedef struct {
    bh_mk_endpoint_entry_t entries[BH_MK_MAX_ENDPOINTS];
} bh_mk_endpoint_table_t;

typedef struct {
    uint64_t tx_messages;
    uint64_t rx_messages;
    uint64_t tx_errors;
    uint64_t rx_errors;
    uint64_t timeouts;
    uint64_t credit_would_blocks;
} bh_mk_diag_t;

typedef struct {
    bh_mk_ring_slot_t control_slots[BH_MK_LANE_CONTROL_CAP];
    bh_mk_ring_slot_t normal_slots[BH_MK_LANE_NORMAL_CAP];
    bh_mk_ring_slot_t bulk_slots[BH_MK_LANE_BULK_CAP];

    bh_mk_mpsc_ring_t control_in;
    bh_mk_mpsc_ring_t normal_in;
    bh_mk_mpsc_ring_t bulk_in;

    bh_mk_tx_table_t txns;
    bh_mk_replay_cache_t replay;
    bh_mk_endpoint_table_t endpoints;
    bh_mk_diag_t diagnostics;

    _Atomic uint32_t ready;
    _Atomic uint32_t generation;
} bh_mk_core_fabric_t;

typedef struct {
    bh_mk_handler_t handler_fn;
    void *ctx;
    uint32_t message_class;
    uint32_t opcode;
} bh_mk_endpoint_config_t;

typedef enum {
    BH_MK_LANE_CONTROL = 0,
    BH_MK_LANE_NORMAL  = 1,
    BH_MK_LANE_BULK    = 2
} bh_mk_lane_t;

typedef struct {
    kstatus_t (*notify)(uint32_t destination_core);
} bh_mk_doorbell_ops_t;

// Globally routable 64-bit endpoint handles packing
#define BH_MK_HANDLE_TYPE_MAGIC 0xA5ULL

#define BH_MK_HANDLE_SLOT_MASK      0xFFFULL
#define BH_MK_HANDLE_SLOT_SHIFT     0U
#define BH_MK_HANDLE_EP_GEN_MASK    0xFFFFULL
#define BH_MK_HANDLE_EP_GEN_SHIFT   12U
#define BH_MK_HANDLE_CORE_GEN_MASK  0xFFFFULL
#define BH_MK_HANDLE_CORE_GEN_SHIFT  28U
#define BH_MK_HANDLE_CORE_ID_MASK   0xFFFULL
#define BH_MK_HANDLE_CORE_ID_SHIFT  44U
#define BH_MK_HANDLE_TYPE_MASK      0xFFULL
#define BH_MK_HANDLE_TYPE_SHIFT     56U

typedef enum {
    BH_MK_ACTION_IDEMPOTENT = 0,
    BH_MK_ACTION_REPLAY_CACHED,
    BH_MK_ACTION_NON_RETRYABLE
} bh_mk_replay_action_t;

static inline kstatus_t bh_mk_handle_pack(
    uint32_t core_id,
    uint32_t core_gen,
    uint32_t ep_gen,
    uint32_t slot,
    uint64_t *out_handle)
{
    if (core_id > BH_MK_HANDLE_CORE_ID_MASK) return K_ERR_INVALID_ARG;
    if (core_gen > BH_MK_HANDLE_CORE_GEN_MASK) return K_ERR_INVALID_ARG;
    if (ep_gen > BH_MK_HANDLE_EP_GEN_MASK) return K_ERR_INVALID_ARG;
    if (slot > BH_MK_HANDLE_SLOT_MASK) return K_ERR_INVALID_ARG;

    *out_handle = ((uint64_t)BH_MK_HANDLE_TYPE_MAGIC << BH_MK_HANDLE_TYPE_SHIFT) |
                  ((uint64_t)core_id << BH_MK_HANDLE_CORE_ID_SHIFT) |
                  ((uint64_t)core_gen << BH_MK_HANDLE_CORE_GEN_SHIFT) |
                  ((uint64_t)ep_gen << BH_MK_HANDLE_EP_GEN_SHIFT) |
                  ((uint64_t)slot << BH_MK_HANDLE_SLOT_SHIFT);
    return K_OK;
}

static inline kstatus_t bh_mk_handle_unpack(
    uint64_t handle,
    uint32_t *core_id,
    uint32_t *core_gen,
    uint32_t *ep_gen,
    uint32_t *slot)
{
    if (handle == BH_MK_ENDPOINT_INVALID) {
        return K_ERR_NOT_FOUND;
    }
    if (((handle >> BH_MK_HANDLE_TYPE_SHIFT) & BH_MK_HANDLE_TYPE_MASK) != BH_MK_HANDLE_TYPE_MAGIC) {
        return K_ERR_CAP_INVALID;
    }
    *core_id = (handle >> BH_MK_HANDLE_CORE_ID_SHIFT) & BH_MK_HANDLE_CORE_ID_MASK;
    *core_gen = (handle >> BH_MK_HANDLE_CORE_GEN_SHIFT) & BH_MK_HANDLE_CORE_GEN_MASK;
    *ep_gen = (handle >> BH_MK_HANDLE_EP_GEN_SHIFT) & BH_MK_HANDLE_EP_GEN_MASK;
    *slot = (handle >> BH_MK_HANDLE_SLOT_SHIFT) & BH_MK_HANDLE_SLOT_MASK;

    if (*core_id >= BHARAT_MAX_CPUS) return K_ERR_INVALID_ARG;
    if (*slot >= BH_MK_MAX_ENDPOINTS) return K_ERR_INVALID_ARG;

    return K_OK;
}

// Endianness converters
static inline uint16_t bh_le16(uint16_t val) {
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    return __builtin_bswap16(val);
#else
    return val;
#endif
}

static inline uint32_t bh_le32(uint32_t val) {
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    return __builtin_bswap32(val);
#else
    return val;
#endif
}

static inline uint64_t bh_le64(uint64_t val) {
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    return __builtin_bswap64(val);
#else
    return val;
#endif
}

static inline void bh_mk_wire_encode(bh_mk_wire_header_v1_t *hdr) {
    hdr->abi_version = bh_le16(hdr->abi_version);
    hdr->header_size = bh_le16(hdr->header_size);
    hdr->message_class = bh_le16(hdr->message_class);
    hdr->opcode = bh_le16(hdr->opcode);
    hdr->flags = bh_le32(hdr->flags);
    hdr->payload_size = bh_le32(hdr->payload_size);
    hdr->src_core = bh_le32(hdr->src_core);
    hdr->dst_core = bh_le32(hdr->dst_core);
    hdr->src_endpoint = bh_le64(hdr->src_endpoint);
    hdr->src_endpoint_generation = bh_le32(hdr->src_endpoint_generation);
    hdr->dst_endpoint = bh_le64(hdr->dst_endpoint);
    hdr->dst_endpoint_generation = bh_le32(hdr->dst_endpoint_generation);
    hdr->txn_slot = bh_le32(hdr->txn_slot);
    hdr->txn_generation = bh_le32(hdr->txn_generation);
    hdr->sequence = bh_le64(hdr->sequence);
    hdr->deadline_ticks = bh_le64(hdr->deadline_ticks);
    hdr->auth_kind = bh_le32(hdr->auth_kind);
    hdr->auth_length = bh_le32(hdr->auth_length);
    hdr->checksum = bh_le32(hdr->checksum);
    hdr->reserved = bh_le32(hdr->reserved);
}

static inline void bh_mk_wire_decode(bh_mk_wire_header_v1_t *hdr) {
    bh_mk_wire_encode(hdr);
}

// New production APIs
kstatus_t bh_mk_fabric_init(uint32_t discovered_core_count);
kstatus_t bh_mk_endpoint_bind(
    const bh_mk_endpoint_config_t *config,
    bh_mk_endpoint_handle_t *out_endpoint);
kstatus_t bh_mk_send(
    bh_mk_endpoint_handle_t source,
    bh_mk_endpoint_handle_t destination,
    uint16_t message_class,
    uint16_t opcode,
    bh_mk_lane_t lane,
    const void *payload,
    uint32_t payload_size,
    bh_mk_tx_handle_t *out_transaction);
kstatus_t bh_mk_drain_local(uint32_t budget);
kstatus_t bh_mk_tx_complete(
    bh_mk_tx_handle_t handle,
    uint32_t expected_source_core,
    bh_mk_endpoint_handle_t expected_source_endpoint,
    kstatus_t result);

void bh_mk_register_doorbell(const bh_mk_doorbell_ops_t *ops);
bh_mk_core_fabric_t *bh_mk_get_core_fabric(uint32_t core_id);

// ────────────────────────────────────────────────────────
// Legacy structures and compatibility wrappers
// ────────────────────────────────────────────────────────

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

// Mark older API deprecated
#define DEPRECATED_API __attribute__((deprecated))

DEPRECATED_API int mk_proto_txn_table_init(void);
DEPRECATED_API int mk_proto_txn_begin(uint64_t txn_id, uint32_t remote_core, uint32_t msg_type, uint64_t deadline_ticks);
DEPRECATED_API int mk_proto_txn_complete(uint64_t txn_id, int result);
DEPRECATED_API int mk_proto_txn_poll_timeouts(uint64_t now_ticks);
DEPRECATED_API int mk_proto_txn_lookup(uint64_t txn_id, mk_proto_txn_entry_t *out_entry);

DEPRECATED_API int mk_proto_send_tracked(mk_channel_t *channel, uint32_t msg_type,
                          void *payload, uint32_t size,
                          uint64_t txn_id, uint64_t deadline_ticks);

int mk_proto_get_policy(uint32_t msg_type, mk_proto_policy_t *out_policy);
int mk_proto_is_idempotent(uint32_t msg_type);
int mk_proto_should_retry(uint32_t msg_type, mk_proto_result_t result,
                          uint32_t retry_count);
mk_txn_state_t mk_proto_result_to_txn_state(mk_proto_result_t result);
uint32_t mk_proto_result_to_reason_code(mk_proto_result_t result);

#endif // BHARAT_MK_PROTO_H
