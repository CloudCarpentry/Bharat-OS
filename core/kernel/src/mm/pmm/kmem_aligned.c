#include "../../include/slab.h"
#include <stddef.h>
#include <stdint.h>

/*
 * kmem_aligned_alloc - Allocates an aligned block of memory.
 *
 * Returns a pointer to a memory block aligned to 'align' bytes.
 * Uses over-allocation to store the original base pointer just before
 * the aligned region for proper freeing via kmem_aligned_free.
 */
void *kmem_aligned_alloc(size_t align, size_t size) {
    if (align < sizeof(void *)) {
        align = sizeof(void *);
    }

    size_t total = size + align - 1 + sizeof(void *);
    void *raw = kmalloc(total);
    if (!raw) {
        return NULL;
    }

    uintptr_t base = (uintptr_t)raw + sizeof(void *);
    uintptr_t aligned = (base + (align - 1)) & ~(uintptr_t)(align - 1);
    ((void **)aligned)[-1] = raw;
    return (void *)aligned;
}

/*
 * kmem_aligned_free - Frees a memory block allocated by kmem_aligned_alloc.
 */
void kmem_aligned_free(void *ptr) {
    if (!ptr) {
        return;
    }
    void *raw = ((void **)ptr)[-1];
    kfree(raw);
}
