#include <bharat/libc/alloc.h>

#define ALIGN_UP(size, align) (((size) + ((align) - 1)) & ~((align) - 1))

typedef struct {
    size_t size;
    int is_free;
} bh_block_hdr_t;

void bh_bounded_freelist_init(bh_bounded_freelist_t *fl, void *buffer, size_t capacity) {
    if (!fl || !buffer || capacity < sizeof(bh_block_hdr_t)) return;

    fl->buffer = (uint8_t *)buffer;
    fl->capacity = capacity;
    fl->free_list = NULL; // unused in this header-in-buffer implementation

    // Set up initial block covering the whole buffer
    bh_block_hdr_t *initial_block = (bh_block_hdr_t *)fl->buffer;
    initial_block->size = capacity - sizeof(bh_block_hdr_t);
    initial_block->is_free = 1;
}

void *bh_bounded_freelist_alloc(bh_bounded_freelist_t *fl, size_t size) {
    if (!fl || !fl->buffer || size == 0) return NULL;

    size_t aligned_size = ALIGN_UP(size, 8);
    size_t offset = 0;

    while (offset < fl->capacity) {
        if (offset + sizeof(bh_block_hdr_t) > fl->capacity) {
            break;
        }
        bh_block_hdr_t *hdr = (bh_block_hdr_t *)(fl->buffer + offset);

        if (hdr->is_free && hdr->size >= aligned_size) {
            // Can we split?
            if (hdr->size >= aligned_size + sizeof(bh_block_hdr_t) + 8) {
                size_t old_size = hdr->size;
                hdr->size = aligned_size;
                hdr->is_free = 0;

                size_t next_offset = offset + sizeof(bh_block_hdr_t) + aligned_size;
                if (next_offset + sizeof(bh_block_hdr_t) <= fl->capacity) {
                    bh_block_hdr_t *next_hdr = (bh_block_hdr_t *)(fl->buffer + next_offset);
                    next_hdr->size = old_size - aligned_size - sizeof(bh_block_hdr_t);
                    next_hdr->is_free = 1;
                }
            } else {
                hdr->is_free = 0;
            }
            return (void *)(fl->buffer + offset + sizeof(bh_block_hdr_t));
        }

        offset += sizeof(bh_block_hdr_t) + hdr->size;
    }

    return NULL; // Out of memory
}

void bh_bounded_freelist_free(bh_bounded_freelist_t *fl, void *ptr) {
    if (!fl || !fl->buffer || !ptr) return;

    uint8_t *p = (uint8_t *)ptr;
    if (p < fl->buffer || p >= fl->buffer + fl->capacity) {
        return; // pointer outside pool
    }

    bh_block_hdr_t *target_hdr = (bh_block_hdr_t *)(p - sizeof(bh_block_hdr_t));
    target_hdr->is_free = 1;

    // Coalesce free blocks
    size_t offset = 0;
    bh_block_hdr_t *prev_hdr = NULL;

    while (offset < fl->capacity) {
        if (offset + sizeof(bh_block_hdr_t) > fl->capacity) {
            break;
        }
        bh_block_hdr_t *hdr = (bh_block_hdr_t *)(fl->buffer + offset);

        if (prev_hdr && prev_hdr->is_free && hdr->is_free) {
            prev_hdr->size += sizeof(bh_block_hdr_t) + hdr->size;
            // Do not advance prev_hdr because it was enlarged and we might coalesce it with the next block
        } else {
            prev_hdr = hdr;
        }
        offset += sizeof(bh_block_hdr_t) + hdr->size;
    }
}
