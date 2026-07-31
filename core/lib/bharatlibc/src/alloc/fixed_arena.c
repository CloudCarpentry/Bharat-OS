#include <bharat/libc/alloc.h>

void bh_fixed_arena_init(bh_fixed_arena_t *arena, void *buffer, size_t capacity) {
    if (!arena) return;
    arena->buffer = (uint8_t *)buffer;
    arena->capacity = capacity;
    arena->offset = 0;
}

void *bh_fixed_arena_alloc(bh_fixed_arena_t *arena, size_t size, size_t alignment) {
    if (!arena || !arena->buffer || size == 0) return NULL;
    if (alignment == 0) alignment = 1;

    // Calculate alignment padding
    uintptr_t current_ptr = (uintptr_t)arena->buffer + arena->offset;
    uintptr_t aligned_ptr = (current_ptr + (alignment - 1)) & ~(alignment - 1);
    size_t new_offset = aligned_ptr - (uintptr_t)arena->buffer + size;

    if (new_offset > arena->capacity) {
        return NULL; // Out of memory / exhausted
    }

    arena->offset = new_offset;
    return (void *)aligned_ptr;
}

void bh_fixed_arena_reset(bh_fixed_arena_t *arena) {
    if (!arena) return;
    arena->offset = 0;
}
