#pragma once

#include <stdint.h>
#include <stdbool.h>

/**
 * @file mcs_lock.h
 * @brief Scalable, local-spin MCS (Mellor-Crummey and Scott) lock.
 *
 * This implementation resolves global lock scalability issues by spinning
 * on per-core/local variables, significantly reducing cache invalidation
 * overheads. It should leverage hardware atomic exchanges (e.g., `XCHG`, `LDXR/STXR`).
 */

/**
 * @struct mcs_qnode_t
 * @brief Local queue node used by the thread trying to acquire the lock.
 */
typedef struct mcs_qnode {
    struct mcs_qnode *next;
    volatile bool locked;
} mcs_qnode_t;

/**
 * @typedef mcs_lock_t
 * @brief The lock structure, essentially a pointer to the tail node.
 */
typedef mcs_qnode_t * volatile mcs_lock_t;

/**
 * @brief Initialize the MCS lock.
 *
 * @param lock Pointer to the lock tail.
 */
static inline void mcs_lock_init(mcs_lock_t *lock) {
    *lock = 0; // NULL
}

/**
 * @brief Acquire the MCS lock.
 *
 * @param lock Pointer to the global lock tail.
 * @param node Pointer to the core-local queue node.
 */
void mcs_lock_acquire(mcs_lock_t *lock, mcs_qnode_t *node);

/**
 * @brief Release the MCS lock.
 *
 * @param lock Pointer to the global lock tail.
 * @param node Pointer to the core-local queue node that acquired the lock.
 */
void mcs_lock_release(mcs_lock_t *lock, mcs_qnode_t *node);
