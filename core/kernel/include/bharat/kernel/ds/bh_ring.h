#ifndef BHARAT_KERNEL_DS_RING_H
#define BHARAT_KERNEL_DS_RING_H

#include <stdint.h>
#include <stdbool.h>
#include <kernel/status.h>

/**
 * @file bh_ring.h
 * @brief Kernel Bounded Ring Buffer
 *
 * Provides a fixed-capacity SPSC (Single-Producer Single-Consumer) ring buffer.
 */

typedef struct bh_ring {
    uint32_t head;
    uint32_t tail;
    uint32_t capacity;     /* must be power of 2 for optimal performance, but not strictly required by this API */
    uint32_t element_size;
    uint8_t *storage;
} bh_ring_t;

/**
 * @brief Initialize a ring buffer.
 *
 * @param ring Pointer to ring structure.
 * @param storage Pointer to pre-allocated storage.
 * @param capacity Number of elements.
 * @param element_size Size of each element in bytes.
 */
void bh_ring_init(bh_ring_t *ring, void *storage, uint32_t capacity, uint32_t element_size);

/**
 * @brief Push an element into the ring.
 * @return K_OK on success, K_ERR_IPC_QUEUE_FULL on overflow.
 */
kstatus_t bh_ring_push(bh_ring_t *ring, const void *data);

/**
 * @brief Pop an element from the ring.
 * @return K_OK on success, K_ERR_NOT_FOUND or similar on underflow.
 */
kstatus_t bh_ring_pop(bh_ring_t *ring, void *data);

/**
 * @brief Check if the ring is empty.
 */
bool bh_ring_is_empty(const bh_ring_t *ring);

/**
 * @brief Check if the ring is full.
 */
bool bh_ring_is_full(const bh_ring_t *ring);

#endif // BHARAT_KERNEL_DS_RING_H
