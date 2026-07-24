#include <bharat/kernel/ds/bh_id_allocator.h>
#include <kernel/status.h>
#include <lib/base/string.h>

kstatus_t bh_id_allocator_init(bh_id_allocator_t *alloc, uint8_t *bitmap, uint32_t capacity) {
    if (!alloc || !bitmap || capacity == 0) {
        return K_ERR_INVALID_ARG;
    }

    alloc->capacity = capacity;
    alloc->bitmap = bitmap;
    alloc->next_hint = 0;
    alloc->allocated_count = 0;

    memset(bitmap, 0, (capacity + 7) / 8);

    return K_OK;
}

kstatus_t bh_id_allocator_alloc(bh_id_allocator_t *alloc, uint32_t *out_id) {
    if (!alloc || !out_id) {
        return K_ERR_INVALID_ARG;
    }

    if (alloc->allocated_count >= alloc->capacity) {
        return K_ERR_PMM_EXHAUSTED;
    }

    uint32_t start = alloc->next_hint;
    for (uint32_t i = 0; i < alloc->capacity; i++) {
        uint32_t id = (start + i) % alloc->capacity;
        uint32_t byte_idx = id / 8;
        uint8_t bit_mask = (1 << (id % 8));

        if (!(alloc->bitmap[byte_idx] & bit_mask)) {
            alloc->bitmap[byte_idx] |= bit_mask;
            alloc->allocated_count++;
            alloc->next_hint = (id + 1) % alloc->capacity;
            *out_id = id;
            return K_OK;
        }
    }

    return K_ERR_PMM_EXHAUSTED;
}

kstatus_t bh_id_allocator_free(bh_id_allocator_t *alloc, uint32_t id) {
    if (!alloc || id >= alloc->capacity) {
        return K_ERR_INVALID_ARG;
    }

    uint32_t byte_idx = id / 8;
    uint8_t bit_mask = (1 << (id % 8));

    if (!(alloc->bitmap[byte_idx] & bit_mask)) {
        return K_ERR_ALREADY_EXISTS; /* Actually "already free" */
    }

    alloc->bitmap[byte_idx] &= ~bit_mask;
    alloc->allocated_count--;
    return K_OK;
}

bool bh_id_allocator_is_allocated(const bh_id_allocator_t *alloc, uint32_t id) {
    if (!alloc || id >= alloc->capacity) {
        return false;
    }

    uint32_t byte_idx = id / 8;
    uint8_t bit_mask = (1 << (id % 8));

    return (alloc->bitmap[byte_idx] & bit_mask) != 0;
}

uint32_t bh_id_allocator_capacity(const bh_id_allocator_t *alloc) {
    return alloc ? alloc->capacity : 0;
}
