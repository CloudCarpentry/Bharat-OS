#ifndef BHARAT_KERNEL_DS_BH_BITMAP_H
#define BHARAT_KERNEL_DS_BH_BITMAP_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <kernel/status.h>

/**
 * @file bh_bitmap.h
 * @brief Fixed-size bitmap over caller-owned storage
 */

typedef struct bh_bitmap {
    uint8_t *bits;
    size_t size_bits;
} bh_bitmap_t;

/**
 * @brief Initialize a bitmap with caller-provided storage
 *
 * @param bitmap Pointer to bitmap structure
 * @param storage Pointer to storage buffer
 * @param size_bits Size of bitmap in bits
 * @return kstatus_t K_OK on success
 */
kstatus_t bh_bitmap_init(bh_bitmap_t *bitmap, void *storage, size_t size_bits);

/**
 * @brief Set a bit in the bitmap
 */
kstatus_t bh_bitmap_set(bh_bitmap_t *bitmap, size_t bit);

/**
 * @brief Clear a bit in the bitmap
 */
kstatus_t bh_bitmap_clear(bh_bitmap_t *bitmap, size_t bit);

/**
 * @brief Test if a bit is set
 */
bool bh_bitmap_test(const bh_bitmap_t *bitmap, size_t bit);

/**
 * @brief Find first clear bit
 */
kstatus_t bh_bitmap_find_first_clear(const bh_bitmap_t *bitmap, size_t *out_bit);

/**
 * @brief Find first set bit
 */
kstatus_t bh_bitmap_find_first_set(const bh_bitmap_t *bitmap, size_t *out_bit);

/**
 * @brief Set a range of bits
 */
kstatus_t bh_bitmap_set_range(bh_bitmap_t *bitmap, size_t start, size_t count);

/**
 * @brief Clear a range of bits
 */
kstatus_t bh_bitmap_clear_range(bh_bitmap_t *bitmap, size_t start, size_t count);

#endif // BHARAT_KERNEL_DS_BH_BITMAP_H
