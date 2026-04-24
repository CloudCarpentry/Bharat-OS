#pragma once

#include <stdint.h>
#include <stddef.h>

/**
 * @file radix_tree.h
 * @brief Lock-free radix tree structure optimized for capability maps.
 *
 * This implementation is intended for lock-free memory mapping and capability
 * delegation/revocation. Readers should use RCU (Read-Copy-Update), while writers
 * require synchronization.
 *
 * Designed to leverage hardware bit manipulation instructions (e.g., `CLZ`, `CTZ`, `BSF/BSR`).
 */

#define RADIX_TREE_MAX_DEPTH 64
#define RADIX_TREE_RADIX     4   /* Bits per level */
#define RADIX_TREE_NODES     (1 << RADIX_TREE_RADIX)

/**
 * @struct radix_node_t
 * @brief Node within the radix tree.
 */
typedef struct radix_node {
    uint16_t valid_children; /* Bitmap of allocated children for fast traversal */
    struct radix_node *children[RADIX_TREE_NODES];
    void *value; /* Value stored at the node (e.g., capability or memory map) */
} radix_node_t;

/**
 * @struct radix_tree_t
 * @brief Root structure for the radix tree.
 */
typedef struct radix_tree {
    radix_node_t *root;
    size_t depth; /* The logical depth, dependent on architecture addressing */
} radix_tree_t;

/**
 * @brief Initialize a new radix tree.
 *
 * @param tree Pointer to the radix tree.
 */
void radix_tree_init(radix_tree_t *tree);

/**
 * @brief Lock-free read operation to find a value by its key.
 *
 * @param tree Pointer to the radix tree.
 * @param key The key to look up (e.g., capability handle).
 * @return The associated value, or NULL if not found.
 */
void *radix_tree_lookup(const radix_tree_t *tree, uint64_t key);

/**
 * @brief Insert a key-value pair into the radix tree.
 *
 * @param tree Pointer to the radix tree.
 * @param key The key to insert.
 * @param value The value to associate with the key.
 * @return 0 on success, or an error code.
 */
int radix_tree_insert(radix_tree_t *tree, uint64_t key, void *value);

/**
 * @brief Remove a key-value pair from the radix tree.
 *
 * @param tree Pointer to the radix tree.
 * @param key The key to remove.
 * @return The value that was removed, or NULL if the key was not found.
 */
void *radix_tree_remove(radix_tree_t *tree, uint64_t key);
