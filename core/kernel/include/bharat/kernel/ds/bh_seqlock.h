#ifndef BHARAT_KERNEL_DS_SEQLOCK_H
#define BHARAT_KERNEL_DS_SEQLOCK_H

#include <stdint.h>
#include <stdbool.h>
#include <kernel/status.h>
#include <atomic.h>
#include <spinlock.h>

/**
 * @file bh_seqlock.h
 * @brief Kernel Sequence Lock Primitive
 *
 * Seqlocks are used for fast, lockless reads of small amounts of data that
 * are updated infrequently. Readers do not block writers.
 *
 * Ordering and Semantics:
 * - Readers are lockless and never sleep.
 * - IRQ context reading is ONLY safe when the writer-side execution
 *   prevents that IRQ from interrupting an active writer (caller responsibility).
 * - Writers are serialized by an internal `writer_lock`.
 * - Writers may not nest (recursive writes will deadlock or assert).
 * - Writers must not sleep or block while holding the lock.
 * - The base seqlock does NOT alter preemption or IRQ state; callers
 *   must provide context exclusion where required.
 * - Sequence wrap is bounded: readers must not remain active across 2^31
 *   complete writer generations.
 */

typedef struct bh_seqlock {
    atomic32_t sequence;
    spinlock_t writer_lock;
} bh_seqlock_t;

/**
 * @brief Initialize a seqlock.
 */
void bh_seqlock_init(bh_seqlock_t *lock);

/**
 * @brief Begin a read-side critical section.
 * @return Current sequence number to be used for retry check.
 */
uint32_t bh_seqlock_read_begin(const bh_seqlock_t *lock);

/**
 * @brief Check if a read-side critical section needs to be retried.
 * @param lock Pointer to the seqlock.
 * @param seq Sequence number returned by bh_seqlock_read_begin.
 * @return true if the data was modified and the read should be retried.
 */
bool bh_seqlock_read_retry(const bh_seqlock_t *lock, uint32_t seq);

/**
 * @brief Begin a write-side critical section.
 *
 * Note: Caller must provide necessary IRQ/preemption exclusion to protect
 * against local readers interrupting this critical section.
 */
void bh_seqlock_write_begin(bh_seqlock_t *lock);

/**
 * @brief End a write-side critical section.
 */
void bh_seqlock_write_end(bh_seqlock_t *lock);

#endif // BHARAT_KERNEL_DS_SEQLOCK_H
