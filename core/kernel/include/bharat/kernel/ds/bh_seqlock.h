#ifndef BHARAT_KERNEL_DS_SEQLOCK_H
#define BHARAT_KERNEL_DS_SEQLOCK_H

#include <stdint.h>
#include <stdbool.h>
#include <kernel/status.h>

/**
 * @file bh_seqlock.h
 * @brief Kernel Sequence Lock Primitive
 *
 * Seqlocks are used for fast, lockless reads of small amounts of data that
 * are updated infrequently. Readers do not block writers.
 */

typedef struct bh_seqlock {
    uint64_t sequence;
} bh_seqlock_t;

/**
 * @brief Initialize a seqlock.
 */
void bh_seqlock_init(bh_seqlock_t *lock);

/**
 * @brief Begin a read-side critical section.
 * @return Current sequence number to be used for retry check.
 */
uint64_t bh_seqlock_read_begin(const bh_seqlock_t *lock);

/**
 * @brief Check if a read-side critical section needs to be retried.
 * @param lock Pointer to the seqlock.
 * @param seq Sequence number returned by bh_seqlock_read_begin.
 * @return true if the data was modified and the read should be retried.
 */
bool bh_seqlock_read_retry(const bh_seqlock_t *lock, uint64_t seq);

/**
 * @brief Begin a write-side critical section.
 *
 * This typically disables preemption/interrupts depending on implementation.
 */
void bh_seqlock_write_begin(bh_seqlock_t *lock);

/**
 * @brief End a write-side critical section.
 */
void bh_seqlock_write_end(bh_seqlock_t *lock);

#endif // BHARAT_KERNEL_DS_SEQLOCK_H
