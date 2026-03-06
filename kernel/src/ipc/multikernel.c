#include "../../include/advanced/multikernel.h"
#include "../../include/atomic.h"
#include "../../include/hal/hal.h"

#include <stddef.h>

/**
 * Bharat-OS Lockless inter-core URPC (User-level Remote Procedure Call)
 * Uses single-producer, single-consumer ring buffers in shared memory to
 * avoid expensive spinlocks when passing state between cores.
 */

#define URPC_SUCCESS 0
#define URPC_ERR_FULL -1
#define URPC_ERR_EMPTY -2

void urpc_init_ring(urpc_ring_t* ring, urpc_msg_t* buffer_ptr, uint32_t ring_size) {
    if (!ring || !buffer_ptr || ring_size == 0U) {
        return;
    }

    ring->buffer = buffer_ptr;
    ring->capacity = ring_size;
    ring->head = 0;
    ring->tail = 0;
}

int urpc_send(urpc_ring_t* ring, urpc_msg_t* msg) {
    if (!ring || !msg) {
        return URPC_ERR_EMPTY;
    }

    // Check if the ring is full before attempting any fastpath or shared memory write
    // This maintains the fail-fast mechanism to prevent deadlocks in highly parallel profiles.
    uint32_t current_head = ring->head;
    uint32_t next_head = (current_head + 1U) % ring->capacity;

    if (next_head == ring->tail) {
        return URPC_ERR_FULL;
    }

    // Optimization for small payloads (<= 64 bits / 8 bytes)
    // Send via register message (architectural fast path)
    if (msg->payload_size <= 8U) {
        // Target core id placeholder (in real implementation extracted from ring metadata)
        uint32_t target_core = 0;
        hal_send_ipi_payload(target_core, msg->payload_data[0]);

        return URPC_SUCCESS; // Bypass shared memory
    }

    // Copy the message into the ring
    ring->buffer[current_head] = *msg;

    // Ensure the message data is visible precisely before updating the head index
    smp_mb();

    ring->head = next_head;

    // In a full implementation, we'd send an IPI to wake a sleeping target core.
    return URPC_SUCCESS;
}

int urpc_receive(urpc_ring_t* ring, urpc_msg_t* out_msg) {
    if (!ring || !out_msg) {
        return URPC_ERR_EMPTY;
    }

    uint32_t current_tail = ring->tail;

    // Check if the ring is empty
    if (current_tail == ring->head) {
        return URPC_ERR_EMPTY; // Nothing to receive
    }

    // Ensure we read the index update before reading the actual message data
    smp_mb();

    // Copy out the message
    *out_msg = ring->buffer[current_tail];

    // Advance tail
    ring->tail = (current_tail + 1U) % ring->capacity;

    return URPC_SUCCESS;
}
