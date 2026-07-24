#ifndef BHARAT_SLAB_H
#define BHARAT_SLAB_H

#include <stdint.h>
#include <stddef.h>
#include "mm.h"

#define KCACHE_MAX_PAGES             64
#define SLAB_MIN_OBJECT_SIZE         16
#define KCACHE_MAX_OBJS_PER_PAGE     (4096 / SLAB_MIN_OBJECT_SIZE) // Assumes PAGE_SIZE is 4096
#define KCACHE_BITMAP_WORDS_PER_PAGE ((KCACHE_MAX_OBJS_PER_PAGE + 31u) / 32u)

// Kernel cache structure for custom object caching
typedef struct kcache {
    const char* name;
    size_t object_size;
    // Basic tracking arrays for simplicity
    phys_addr_t pages[KCACHE_MAX_PAGES];
    uint32_t num_pages;

    uint32_t objs_per_page;
    uint32_t bitmap_words_per_page;

    // Bitmap for allocated items (1 = allocated)
    uint32_t free_bitmap[KCACHE_MAX_PAGES][KCACHE_BITMAP_WORDS_PER_PAGE];
} kcache_t;

// Standard kernel heap allocator
void* kmalloc(size_t size);
void* kzalloc(size_t size);
void kfree(void* ptr);

// Aligned kernel memory allocator
void *kmem_aligned_alloc(size_t align, size_t size);
void kmem_aligned_free(void *ptr);

// Custom size cache allocator
kcache_t* kcache_create(const char* name, size_t size);
void* kcache_alloc(kcache_t* cache);
void kcache_free(kcache_t* cache, void* obj);

// Virtual address allocator for large contiguous segments
void* kvmalloc(size_t size);
void kvfree(void* ptr);

#endif
