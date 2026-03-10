#ifndef BHARAT_MULTIKERNEL_H
#define BHARAT_MULTIKERNEL_H

#if __has_include("bharat_config.h")
#include "bharat_config.h"
#endif

#include "../sched.h"
#include <stdint.h>

// Fallback alignment macro if not defined by config system
#ifndef BHARAT_ALIGNED_CACHE
#define BHARAT_ALIGNED_CACHE __attribute__((aligned(64)))
#endif

/*
 * Bharat-OS Multikernel Architecture (Barrelfish Inspired)
 * Replaces traditional shared-state kernels with a distributed message-passing
 * system running independently on each physical CPU core to allow lockless
 * scaling on massive hardware.
 */

typedef struct {
  uint32_t msg_type;
  uint32_t payload_size;
  uint32_t sender_core_id;
  uint32_t receiver_core_id;
  uint64_t msg_id;
  uint64_t txn_id;
  uint32_t flags;
  uint32_t auth_token;
  uint64_t payload_data[8]; // Max 64 bytes in shared memory ring
} urpc_msg_t;

typedef enum {
  URPC_SUCCESS = 0,
  URPC_SUCCESS_WOKE = 1,
  URPC_ERR_FULL = -1,
  URPC_ERR_EMPTY = -2,
  URPC_ERR_INVALID = -3,
  URPC_ERR_NO_CHANNEL = -4,
  URPC_ERR_INVAL = -5, // To match the review suggestion
} urpc_status_t;

#include <stdatomic.h>

// Single-Producer / Single-Consumer (SPSC) lockless ring buffer.
// Queue ownership rules:
// - The Producer exclusively owns and updates the `head` index.
// - The Consumer exclusively owns and updates the `tail` index.
// - Memory ordering relies on C11 acquire/release semantics rather than seq_cst.
typedef struct {
  urpc_msg_t *buffer;
  uint32_t capacity;
  _Atomic uint32_t head BHARAT_ALIGNED_CACHE; // Producer owned: Write-published with release
  _Atomic uint32_t tail BHARAT_ALIGNED_CACHE; // Consumer owned: Read-observed with acquire
} BHARAT_ALIGNED_CACHE urpc_ring_t;

typedef struct {
  uint8_t in_use;
  urpc_msg_t msg;
} mk_message_slot_t;

typedef struct {
  mk_message_slot_t* slots;
  uint32_t capacity;
} mk_msg_pool_t;

// A Single-Producer / Single-Consumer (SPSC) Message Channel
// connecting two independent kernel instances on different cores.
// Currently mapped in a core-to-core topology matrix for Phase 1.
// Per-channel state machine
typedef enum {
  MK_CHANNEL_STATE_RESET,
  MK_CHANNEL_STATE_INIT,
  MK_CHANNEL_STATE_ESTABLISHED,
  MK_CHANNEL_STATE_DEGRADED,
  MK_CHANNEL_STATE_DRAINING,
  MK_CHANNEL_STATE_FAILED
} mk_channel_state_t;

// Transaction state machine
typedef enum {
  MK_TXN_STATE_FREE,
  MK_TXN_STATE_PENDING_SEND,
  MK_TXN_STATE_SENT,
  MK_TXN_STATE_ACKED,
  MK_TXN_STATE_REPLIED,
  MK_TXN_STATE_TIMED_OUT,
  MK_TXN_STATE_CANCELLED
} mk_txn_state_t;

typedef struct {
  uint32_t sender_core_id;
  uint32_t receiver_core_id;
  mk_channel_state_t state;

  // Lockless URPC (User-level Remote Procedure Call) Ring Buffer
  // Mapped in cache-aligned shared memory between the two cores
  urpc_ring_t *urpc_ring;
  uint32_t ring_size;
} BHARAT_ALIGNED_CACHE mk_channel_t;

// Transaction tracking structure
typedef struct {
  uint64_t txn_id;
  uint32_t remote_core;
  uint32_t msg_type;
  mk_txn_state_t state;
  uint64_t timeout_deadline;
  uint32_t retry_count;
  int completion_status;
  // Could hold a reply buffer or continuation
} mk_txn_t;

// Message descriptor table entry
typedef struct {
  uint32_t msg_type;
  uint32_t flags;
  uint32_t min_payload_size;
  uint32_t max_payload_size;
  int (*handler_fn)(mk_channel_t *channel, urpc_msg_t *msg);
  int reply_required;
  int idempotent;
  int owner_validation_required;
} mk_msg_desc_t;

// Built-in Control Messages (Transport Layer)
#define MK_MSG_TYPE_ACK 0x1000
#define MK_MSG_TYPE_NACK 0x1001

// Ownership RPC Messages
#define MK_MSG_FRAME_ALLOC_REQ    0x2000
#define MK_MSG_FRAME_FREE_REQ     0x2001
#define MK_MSG_FRAME_MAP_REQ      0x2002
#define MK_MSG_FRAME_UNMAP_REQ    0x2003

#define MK_MSG_PROC_CREATE_REQ    0x3000
#define MK_MSG_PROC_DESTROY_REQ   0x3001
#define MK_MSG_PROC_SIGNAL_REQ    0x3002

#define MK_MSG_ASPACE_CREATE_REQ  0x4000
#define MK_MSG_ASPACE_MAP_REQ     0x4001
#define MK_MSG_ASPACE_PROTECT_REQ 0x4002
#define MK_MSG_ASPACE_UNMAP_REQ   0x4003

#define MK_MSG_CAP_GRANT_REQ      0x5000
#define MK_MSG_CAP_REVOKE_REQ     0x5001
#define MK_MSG_CAP_DERIVE_REQ     0x5002
#define MK_MSG_CAP_LOOKUP_REQ     0x5003

// Multicore boot and channel matrix setup
int mk_boot_secondary_cores(uint32_t core_count);
int mk_init_per_core_channels(uint32_t core_count, uint32_t ring_size);
int mk_get_channel(uint32_t sender_core, uint32_t receiver_core, mk_channel_t* out_channel);

// Establishes a high-throughput lockless IPC channel between two CPU cores
int mk_establish_channel(uint32_t target_core, mk_channel_t *out_channel);

// Send an asynchronous state-update message to a remote CPU core's OS instance
int mk_send_message(mk_channel_t *channel, uint32_t msg_type, void *payload,
                    uint32_t size);

// Poll the core-local URPC ring for incoming messages from other OS instances
int mk_poll_messages(mk_channel_t *channel);

// Scalable message pool allocator used by core-local URPC producers
int mk_msg_pool_init(mk_msg_pool_t* pool, mk_message_slot_t* slots, uint32_t capacity);
urpc_msg_t* mk_msg_alloc(mk_msg_pool_t* pool);
void mk_msg_free(mk_msg_pool_t* pool, urpc_msg_t* msg);

// Low-level Lockless URPC messaging spine API
int urpc_init_ring(urpc_ring_t* ring, urpc_msg_t* buffer_ptr, uint32_t ring_size);
int urpc_send(urpc_ring_t* ring, const urpc_msg_t* msg);
int urpc_receive(urpc_ring_t* ring, urpc_msg_t* out_msg);

#endif // BHARAT_MULTIKERNEL_H
