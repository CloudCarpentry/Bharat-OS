#ifndef BHARAT_KERNEL_DS_BH_REFCOUNT_H
#define BHARAT_KERNEL_DS_BH_REFCOUNT_H

#include "atomic.h"
#include "kernel/status.h"
#include <stdbool.h>

/**
 * @file bh_refcount.h
 * @brief Canonical 32-bit kernel reference counting primitive.
 *
 * This primitive provides safe reference counting with the following properties:
 * - No resurrection from zero: Incrementing a zero refcount is an error.
 * - Saturation: Incrementing at UINT32_MAX fails instead of wrapping.
 * - Underflow detection: Decrementing from zero is an error.
 * - No hidden locks: Uses hardware atomics.
 * - No dynamic allocation.
 */

typedef struct {
    volatile uint32_t value;
} bh_refcount_t;

/**
 * @brief Initialize a refcount with an explicit initial value.
 * @param ref Pointer to refcount structure.
 * @param initial Initial value (e.g., 1 for a new object, 0 for free).
 */
void bh_refcount_init(bh_refcount_t *ref, uint32_t initial);

/**
 * @brief Read the current refcount value.
 * @param ref Pointer to refcount structure.
 * @return Current value.
 */
uint32_t bh_refcount_read(const bh_refcount_t *ref);

/**
 * @brief Attempt to increment the refcount.
 * Fails if the count is zero (cannot resurrect) or if it would overflow.
 * @param ref Pointer to refcount structure.
 * @return K_OK on success, K_ERR_BAD_STATE if zero, K_ERR_OVERFLOW if at max.
 */
kstatus_t bh_refcount_try_inc(bh_refcount_t *ref);

/**
 * @brief Explicit alias for bh_refcount_try_inc.
 */
kstatus_t bh_refcount_try_inc_not_zero(bh_refcount_t *ref);

/**
 * @brief Decrement the refcount and check if it reached zero.
 * @param ref Pointer to refcount structure.
 * @param is_zero Out parameter, set to true if the count reached zero.
 * @return K_OK on success, K_ERR_BAD_STATE if already zero (underflow).
 */
kstatus_t bh_refcount_dec_and_test(bh_refcount_t *ref, bool *is_zero);

/**
 * @brief Check if the refcount is zero.
 * @param ref Pointer to refcount structure.
 * @return True if zero, false otherwise.
 */
bool bh_refcount_is_zero(const bh_refcount_t *ref);

#endif // BHARAT_KERNEL_DS_BH_REFCOUNT_H
