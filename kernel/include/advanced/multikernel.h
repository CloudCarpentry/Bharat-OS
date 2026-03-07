#ifndef BHARAT_MULTIKERNEL_H
#define BHARAT_MULTIKERNEL_H

#include "../sched.h"
#include <stdint.h>

/*
 * Bharat-OS Multikernel Architecture (Barrelfish Inspired)
 * Replaces traditional shared-state kernels with a distributed message-passing
 * system running independently on each physical CPU core to allow lockless
 * scaling on massive hardware.
 */

typedef struct {
  uint32_t msg_type;
  uint32_t payload_size;
  uint64_t payload_data[8]; // Max 64 bytes in shared memory ring
} urpc_msg_t;

typedef enum {
  URPC_SUCCESS = 0,
  URPC_ERR_FULL = -1,
  URPC_ERR_EMPTY = -2,
  URPC_ERR_INVALID = -3,
  URPC_ERR_NO_CHANNEL = -4,
  URPC_ERR_INVAL = -5, // To match the review suggestion
} urpc_status_t;

typedef struct {
  urpc_msg_t *buffer;
  uint32_t capacity;
  volatile uint32_t head;
  volatile uint32_t tail;
} urpc_ring_t;

typedef struct {
  uint8_t in_use;
  urpc_msg_t msg;
} mk_message_slot_t;

typedef struct {
  mk_message_slot_t* slots;
  uint32_t capacity;
} mk_msg_pool_t;

// A Message Channel connecting two independent kernel instances on different
// cores
typedef struct {
  uint32_t sender_core_id;
  uint32_t receiver_core_id;

  // Lockless URPC (User-level Remote Procedure Call) Ring Buffer
  // Mapped in cache-aligned shared memory between the two cores
  urpc_ring_t *urpc_ring;
  uint32_t ring_size;
} mk_channel_t;

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
