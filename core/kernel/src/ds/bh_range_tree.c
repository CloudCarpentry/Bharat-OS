#include <bharat/kernel/ds/bh_range_tree.h>
#include <kernel/status.h>
#include <lib/base/string.h>

void bh_range_tree_init(bh_range_tree_t *tree, bh_range_t *storage, uint32_t capacity) {
    tree->ranges = storage;
    tree->capacity = capacity;
    tree->count = 0;
}

bool bh_range_tree_has_overlap(const bh_range_tree_t *tree, uint64_t start, uint64_t size) {
    uint64_t end = start + size;
    for (uint32_t i = 0; i < tree->count; i++) {
        uint64_t r_start = tree->ranges[i].start;
        uint64_t r_end = r_start + tree->ranges[i].size;

        if (start < r_end && end > r_start) {
            return true;
        }
    }
    return false;
}

kstatus_t bh_range_tree_insert(bh_range_tree_t *tree, uint64_t start, uint64_t size, void *data) {
    if (tree->count >= tree->capacity) {
        return K_ERR_NO_MEMORY;
    }

    if (bh_range_tree_has_overlap(tree, start, size)) {
        return K_ERR_ALREADY_EXISTS;
    }

    /* Find insertion point to keep sorted by start address */
    uint32_t i;
    for (i = 0; i < tree->count; i++) {
        if (tree->ranges[i].start > start) {
            break;
        }
    }

    /* Shift elements */
    for (uint32_t j = tree->count; j > i; j--) {
        tree->ranges[j] = tree->ranges[j - 1];
    }

    tree->ranges[i].start = start;
    tree->ranges[i].size = size;
    tree->ranges[i].data = data;
    tree->count++;

    return K_OK;
}

kstatus_t bh_range_tree_remove(bh_range_tree_t *tree, uint64_t start) {
    for (uint32_t i = 0; i < tree->count; i++) {
        if (tree->ranges[i].start == start) {
            /* Shift elements back */
            for (uint32_t j = i; j < tree->count - 1; j++) {
                tree->ranges[j] = tree->ranges[j + 1];
            }
            tree->count--;
            return K_OK;
        }
    }
    return K_ERR_NOT_FOUND;
}

kstatus_t bh_range_tree_find_by_addr(const bh_range_tree_t *tree, uint64_t addr, bh_range_t *out_range) {
    /* Binary search could be used here since it's sorted */
    for (uint32_t i = 0; i < tree->count; i++) {
        if (addr >= tree->ranges[i].start && addr < (tree->ranges[i].start + tree->ranges[i].size)) {
            if (out_range) {
                *out_range = tree->ranges[i];
            }
            return K_OK;
        }
    }
    return K_ERR_NOT_FOUND;
}

bool bh_range_tree_validate(const bh_range_tree_t *tree) {
    for (uint32_t i = 0; i < tree->count; i++) {
        /* Check size > 0 */
        if (tree->ranges[i].size == 0) return false;

        /* Check sorted and non-overlapping */
        if (i > 0) {
            if (tree->ranges[i].start < (tree->ranges[i-1].start + tree->ranges[i-1].size)) {
                return false;
            }
        }
    }
    return true;
}
