#pragma once

#include <stdint.h>
#include <stddef.h>

/**
 * @file urpc_ring.h
 * @brief Lock-free Multi-Producer Multi-Consumer (MPMC) Ring Buffer.
 *
 * This implementation is designed for per-core queues, such as uRPC or
 * task migration queues. It leverages compiler atomics as a fallback for
 * hardware-specific CAS or LL/SC operations.
 *
 * Note: Hardware optimizations like prefetching and SIMD batching should
 * be integrated at the architecture-specific HAL layer.
 */

#define URPC_RING_SIZE 1024
#define URPC_MSG_SIZE  64

/**
 * @struct urpc_ring_t
 * @brief Cache-aligned MPMC ring buffer structure.
 */
typedef struct {
    uint32_t head __attribute__((aligned(64))); /**< Cache-line aligned head */
    uint32_t tail __attribute__((aligned(64))); /**< Cache-line aligned tail */
    char buffer[URPC_RING_SIZE * URPC_MSG_SIZE] __attribute__((aligned(64)));
} urpc_ring_t;

/**
 * @brief Initialize the uRPC ring buffer.
 *
 * @param ring Pointer to the ring buffer.
 */
void urpc_ring_init(urpc_ring_t *ring);

/**
 * @brief Lock-free send (produce) a message into the ring.
 *
 * @param ring Pointer to the ring buffer.
 * @param msg  Pointer to the message to send (must be URPC_MSG_SIZE bytes).
 * @return 0 on success, or an error code if the ring is full.
 */
int urpc_ring_send(urpc_ring_t *ring, const void *msg);

/**
 * @brief Lock-free receive (consume) a message from the ring.
 *
 * @param ring Pointer to the ring buffer.
 * @param msg_out Buffer to copy the received message into (must be URPC_MSG_SIZE bytes).
 * @return 0 on success, or an error code if the ring is empty.
 */
int urpc_ring_recv(urpc_ring_t *ring, void *msg_out);
