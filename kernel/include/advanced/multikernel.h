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

typedef struct {
  urpc_msg_t *buffer;
  uint32_t capacity;
  volatile uint32_t head;
  volatile uint32_t tail;
} urpc_ring_t;

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

// Establishes a high-throughput lockless IPC channel between two CPU cores
int mk_establish_channel(uint32_t target_core, mk_channel_t *out_channel);

// Send an asynchronous state-update message to a remote CPU core's OS instance
int mk_send_message(mk_channel_t *channel, uint32_t msg_type, void *payload,
                    uint32_t size);

// Poll the core-local URPC ring for incoming messages from other OS instances
int mk_poll_messages(mk_channel_t *channel);

// Low-level Lockless URPC messaging spine API
void urpc_init_ring(urpc_ring_t* ring, urpc_msg_t* buffer_ptr, uint32_t ring_size);
int urpc_send(urpc_ring_t* ring, urpc_msg_t* msg);
int urpc_receive(urpc_ring_t* ring, urpc_msg_t* out_msg);

#endif // BHARAT_MULTIKERNEL_H
