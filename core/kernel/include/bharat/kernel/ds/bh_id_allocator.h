#ifndef BHARAT_KERNEL_DS_ID_ALLOCATOR_H
#define BHARAT_KERNEL_DS_ID_ALLOCATOR_H

#include <stdint.h>
#include <stdbool.h>
#include <kernel/status.h>

/**
 * @file bh_id_allocator.h
 * @brief Kernel ID Allocator
 *
 * Provides a fixed-capacity ID allocator for capabilities, endpoints, etc.
 * Uses caller-provided storage for bitmap or tracking table.
 */

#define BH_ID_ALLOCATOR_INVALID_ID ((uint32_t)-1)

typedef struct bh_id_allocator {
    uint32_t capacity;
    uint32_t next_hint;
    uint32_t allocated_count;
    uint8_t *bitmap;
} bh_id_allocator_t;

/**
 * @brief Initialize the ID allocator.
 *
 * @param alloc Pointer to the allocator structure.
 * @param bitmap Pointer to the bitmap storage (must be at least capacity/8 bytes).
 * @param capacity Maximum number of IDs to manage.
 * @return K_OK on success.
 */
kstatus_t bh_id_allocator_init(bh_id_allocator_t *alloc, uint8_t *bitmap, uint32_t capacity);

/**
 * @brief Allocate a new ID.
 *
 * @param alloc Pointer to the allocator.
 * @param out_id Pointer to store the allocated ID.
 * @return K_OK on success, or K_ERR_PMM_EXHAUSTED if full.
 */
kstatus_t bh_id_allocator_alloc(bh_id_allocator_t *alloc, uint32_t *out_id);

/**
 * @brief Free an allocated ID.
 *
 * @param alloc Pointer to the allocator.
 * @param id The ID to free.
 * @return K_OK on success, K_ERR_INVALID_ARG if ID is out of bounds,
 *         or K_ERR_ALREADY_EXISTS (or similar) if already free.
 */
kstatus_t bh_id_allocator_free(bh_id_allocator_t *alloc, uint32_t id);

/**
 * @brief Check if an ID is currently allocated.
 */
bool bh_id_allocator_is_allocated(const bh_id_allocator_t *alloc, uint32_t id);

/**
 * @brief Get total capacity of the allocator.
 */
uint32_t bh_id_allocator_capacity(const bh_id_allocator_t *alloc);

#endif // BHARAT_KERNEL_DS_ID_ALLOCATOR_H
