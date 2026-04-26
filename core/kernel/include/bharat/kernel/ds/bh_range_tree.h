#ifndef BHARAT_KERNEL_DS_RANGE_TREE_H
#define BHARAT_KERNEL_DS_RANGE_TREE_H

#include <stdint.h>
#include <stdbool.h>
#include <kernel/status.h>

/**
 * @file bh_range_tree.h
 * @brief Kernel Range Lookup Abstract API
 *
 * Provides an abstract interface for managing and looking up non-overlapping ranges.
 * Initial implementation uses a sorted table/array.
 */

typedef struct bh_range {
    uint64_t start;
    uint64_t size;
    void *data;
} bh_range_t;

typedef struct bh_range_tree {
    bh_range_t *ranges;
    uint32_t capacity;
    uint32_t count;
} bh_range_tree_t;

/**
 * @brief Initialize the range tree (table).
 *
 * @param tree Pointer to the tree structure.
 * @param storage Pointer to an array of bh_range_t for internal storage.
 * @param capacity Maximum number of ranges that can be stored.
 */
void bh_range_tree_init(bh_range_tree_t *tree, bh_range_t *storage, uint32_t capacity);

/**
 * @brief Insert a new range.
 *
 * @return K_OK on success, K_ERR_ALREADY_EXISTS if overlaps with existing range,
 *         or K_ERR_NO_MEMORY if capacity reached.
 */
kstatus_t bh_range_tree_insert(bh_range_tree_t *tree, uint64_t start, uint64_t size, void *data);

/**
 * @brief Remove a range starting at the given address.
 *
 * @return K_OK on success, or K_ERR_NOT_FOUND.
 */
kstatus_t bh_range_tree_remove(bh_range_tree_t *tree, uint64_t start);

/**
 * @brief Find the range containing the given address.
 *
 * @param tree Pointer to the tree.
 * @param addr Address to look up.
 * @param out_range Pointer to store the found range (optional).
 * @return K_OK if found, or K_ERR_NOT_FOUND.
 */
kstatus_t bh_range_tree_find_by_addr(const bh_range_tree_t *tree, uint64_t addr, bh_range_t *out_range);

/**
 * @brief Check if a range overlaps with any existing ranges.
 */
bool bh_range_tree_has_overlap(const bh_range_tree_t *tree, uint64_t start, uint64_t size);

/**
 * @brief Validate internal invariants of the tree/table.
 */
bool bh_range_tree_validate(const bh_range_tree_t *tree);

#endif // BHARAT_KERNEL_DS_RANGE_TREE_H
