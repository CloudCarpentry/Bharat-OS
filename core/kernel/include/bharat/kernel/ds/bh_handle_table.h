#ifndef BHARAT_KERNEL_DS_BH_HANDLE_TABLE_H
#define BHARAT_KERNEL_DS_BH_HANDLE_TABLE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <kernel/status.h>

/**
 * @file bh_handle_table.h
 * @brief Fixed-capacity generation handles
 */

typedef uint64_t bh_handle_t;

typedef struct bh_handle_slot {
    uint32_t generation;
    uint32_t type;
    void *object;
    uint64_t rights;
    bool active;
} bh_handle_slot_t;

typedef struct bh_handle_table {
    bh_handle_slot_t *slots;
    size_t capacity;
    size_t count;
} bh_handle_table_t;

/**
 * @brief Initialize a handle table
 */
kstatus_t bh_handle_table_init(bh_handle_table_t *table, void *storage, size_t capacity);

/**
 * @brief Allocate a handle for an object
 */
kstatus_t bh_handle_alloc(bh_handle_table_t *table, void *object, uint32_t type, uint64_t rights, bh_handle_t *out_handle);

/**
 * @brief Lookup an object by handle
 */
kstatus_t bh_handle_lookup(const bh_handle_table_t *table, bh_handle_t handle, uint32_t expected_type, void **out_object, uint64_t *out_rights);

/**
 * @brief Revoke a handle
 */
kstatus_t bh_handle_revoke(bh_handle_table_t *table, bh_handle_t handle);

/**
 * @brief Validate a handle (checks generation and active status)
 */
bool bh_handle_validate(const bh_handle_table_t *table, bh_handle_t handle);

/**
 * Helper to construct a handle from index and generation
 */
static inline bh_handle_t bh_handle_make(uint32_t index, uint32_t generation) {
    return ((uint64_t)generation << 32) | index;
}

/**
 * Helper to extract index from handle
 */
static inline uint32_t bh_handle_index(bh_handle_t handle) {
    return (uint32_t)(handle & 0xFFFFFFFF);
}

/**
 * Helper to extract generation from handle
 */
static inline uint32_t bh_handle_generation(bh_handle_t handle) {
    return (uint32_t)(handle >> 32);
}

#endif // BHARAT_KERNEL_DS_BH_HANDLE_TABLE_H
