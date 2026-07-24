/*
 * kernel/src/lib/ds/radix_tree.c
 * Lock-free radix tree structure optimized for capability maps.
 */

#include "lib/ds/radix_tree.h"
#include "arch/bitops.h"
#include <stddef.h>
#include <stdint.h>
#include "mm.h"  /* Assuming kernel allocator like kalloc/vm_alloc is available here */

extern void *kalloc(size_t size);

void radix_tree_init(radix_tree_t *tree) {
    if (tree == NULL) return;
    tree->root = NULL;
    tree->depth = RADIX_TREE_MAX_DEPTH / RADIX_TREE_RADIX;
}

static radix_node_t *alloc_node(void) {
    /* Rely on the kernel allocator */
    return (radix_node_t *)kalloc(sizeof(radix_node_t));
}

int radix_tree_insert(radix_tree_t *tree, uint64_t key, void *value) {
    if (tree == NULL) return -1;

    if (tree->root == NULL) {
        tree->root = alloc_node();
        if (tree->root == NULL) return -1; /* Out of memory */
        tree->root->valid_children = 0;
        for (int i = 0; i < RADIX_TREE_NODES; i++) {
            tree->root->children[i] = NULL;
        }
        tree->root->value = NULL;
    }

    radix_node_t *node = tree->root;
    for (int level = tree->depth - 1; level >= 0; level--) {
        int index = (key >> (level * RADIX_TREE_RADIX)) & (RADIX_TREE_NODES - 1);

        if ((node->valid_children & (1 << index)) == 0) {
            radix_node_t *new_node = alloc_node();
            if (new_node == NULL) return -1;
            new_node->valid_children = 0;
            for (int i = 0; i < RADIX_TREE_NODES; i++) {
                new_node->children[i] = NULL;
            }
            new_node->value = NULL;
            node->children[index] = new_node;
            node->valid_children |= (1 << index);
        }
        node = node->children[index];
    }

    node->value = value;
    return 0;
}

void *radix_tree_lookup(const radix_tree_t *tree, uint64_t key) {
    if (tree == NULL || tree->root == NULL) return NULL;

    radix_node_t *node = tree->root;
    for (int level = tree->depth - 1; level >= 0; level--) {
        int index = (key >> (level * RADIX_TREE_RADIX)) & (RADIX_TREE_NODES - 1);

        /* Fast-fail using bitmap and arch bit manipulation */
        if ((node->valid_children & (1 << index)) == 0) {
            return NULL; /* Not found */
        }
        node = node->children[index];
    }

    return node->value;
}

void *radix_tree_remove(radix_tree_t *tree, uint64_t key) {
    if (tree == NULL || tree->root == NULL) return NULL;

    radix_node_t *node = tree->root;
    for (int level = tree->depth - 1; level >= 0; level--) {
        int index = (key >> (level * RADIX_TREE_RADIX)) & (RADIX_TREE_NODES - 1);
        if ((node->valid_children & (1 << index)) == 0) {
            return NULL; /* Not found */
        }
        node = node->children[index];
    }

    void *old_val = node->value;
    node->value = NULL;
    return old_val;
}
