#ifndef BHARAT_KERNEL_DS_BH_MPSC_QUEUE_H
#define BHARAT_KERNEL_DS_BH_MPSC_QUEUE_H

#include <stdint.h>
#include <stdbool.h>
#include "kernel/status.h"
#include "atomic.h"

/**
 * @file bh_mpsc_queue.h
 * @brief Bounded Multi-Producer Single-Consumer (MPSC) Lock-Free Queue.
 *
 * This primitive provides a bounded, lock-free queue suitable for cross-core
 * communication where multiple producers (e.g., different cores or IRQ handlers)
 * send commands or requests to a single consumer (e.g., a specific core).
 *
 * Design:
 * - Bounded array-based ring buffer.
 * - Lock-free for both producers and the single consumer.
 * - No heap allocation (caller provides storage).
 * - Capacity must be a power of two (min 2).
 * - Memory ordering is enforced using C11-style atomics/barriers.
 *
 * Usage:
 * 1. Allocate an array of bh_mpsc_slot_t of size N (power of 2).
 * 2. bh_mpsc_queue_init(&q, slots, N);
 * 3. Producers call bh_mpsc_queue_push(&q, value);
 * 4. Consumer calls bh_mpsc_queue_pop(&q, &value);
 */

typedef struct {
    /** @brief Slot sequence number for synchronization. */
    volatile uint64_t seq;
    /** @brief Pointer to the payload. */
    void *value;
} bh_mpsc_slot_t;

typedef struct {
    /** @brief Pointer to the array of slots. */
    bh_mpsc_slot_t *slots;
    /** @brief Capacity of the queue (must be power of two). */
    uint32_t capacity;
    /** @brief Mask for index wrap-around (capacity - 1). */
    uint32_t mask;
    /** @brief Producer head index (shared/atomic). */
    volatile uint64_t head;
    /** @brief Consumer tail index (owned by single consumer). */
    uint64_t tail;
} bh_mpsc_queue_t;

/**
 * @brief Initialize the MPSC queue.
 * @param q Pointer to the queue structure.
 * @param slots Pointer to the pre-allocated array of slots.
 * @param capacity Number of slots (must be a power of two >= 2).
 * @return K_OK on success, K_ERR_INVALID_ARG if capacity is invalid.
 */
kstatus_t bh_mpsc_queue_init(bh_mpsc_queue_t *q, bh_mpsc_slot_t *slots, uint32_t capacity);

/**
 * @brief Push a value into the queue (Multi-Producer safe).
 * @param q Pointer to the queue.
 * @param value Pointer to the payload to push.
 * @return K_OK on success, K_ERR_AGAIN if the queue is full.
 */
kstatus_t bh_mpsc_queue_push(bh_mpsc_queue_t *q, void *value);

/**
 * @brief Pop a value from the queue (Single-Consumer only).
 * @param q Pointer to the queue.
 * @param out_value Pointer to receive the payload.
 * @return K_OK on success, K_ERR_AGAIN if the queue is empty.
 */
kstatus_t bh_mpsc_queue_pop(bh_mpsc_queue_t *q, void **out_value);

/**
 * @brief Check if the queue is empty.
 */
bool bh_mpsc_queue_empty(const bh_mpsc_queue_t *q);

/**
 * @brief Get the capacity of the queue.
 */
uint32_t bh_mpsc_queue_capacity(const bh_mpsc_queue_t *q);

#endif // BHARAT_KERNEL_DS_BH_MPSC_QUEUE_H
