#ifndef BHARAT_SLAB_H
#define BHARAT_SLAB_H

#include <stdint.h>
#include <stddef.h>
#include "mm.h"

// Kernel cache structure for custom object caching
typedef struct kcache {
    const char* name;
    size_t object_size;
    // Basic tracking arrays for simplicity
    phys_addr_t pages[64];
    uint32_t num_pages;
    // Bitmap for free items
    uint32_t free_bitmap[64];
} kcache_t;

// Standard kernel heap allocator
void* kmalloc(size_t size);
void kfree(void* ptr);

// Custom size cache allocator
kcache_t* kcache_create(const char* name, size_t size);
void* kcache_alloc(kcache_t* cache);
void kcache_free(kcache_t* cache, void* obj);

// Virtual address allocator for large contiguous segments
void* kvmalloc(size_t size);
void kvfree(void* ptr);

#endif
