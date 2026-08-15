/** @file slob.c \brief SLOB memory allocator implementation. */

#include "../../include/slab.h"
#include "../../include/mm.h"
#include "../../include/numa.h"
#include "../../include/mm/physmap.h"
#include <stddef.h>
#include "lib/base/string.h"

// SLOB (Simple List Of Blocks) Implementation
// Uses a simple linked list of free blocks within allocated pages.
// Highly space-efficient but slower, typical for embedded systems.

typedef struct slob_block {
    int units;
    struct slob_block *next;
} slob_block_t;

#define SLOB_UNIT sizeof(slob_block_t)
#define SLOB_UNITS_PER_PAGE (PAGE_SIZE / SLOB_UNIT)

static slob_block_t slob_arena = { .units = 0, .next = NULL };
static slob_block_t *slob_free_list = &slob_arena;
static int slob_initialized = 0;

static void slob_add_page(phys_addr_t paddr) {
    void *vptr = physmap_phys_to_virt(paddr);
    slob_block_t *b = (slob_block_t *)(vptr ? vptr : (void *)(uintptr_t)paddr);
    b->units = SLOB_UNITS_PER_PAGE;
    b->next = slob_free_list->next;
    slob_free_list->next = b;
}

static void init_slob() {
    if (slob_initialized) return;
    slob_free_list = &slob_arena;
    slob_free_list->next = NULL;
    slob_initialized = 1;
}

void* kmalloc(size_t size) {
    if (!slob_initialized) init_slob();

    if (size == 0) return NULL;

    if (size > PAGE_SIZE - SLOB_UNIT) {
        int order = 0;
        size_t s = PAGE_SIZE;
        while (s < size) {
            order++;
            s *= 2;
        }
        phys_addr_t p = mm_alloc_pages_order(order, NUMA_NODE_ANY, PAGE_FLAG_KERNEL);
        if (!p) return NULL;
        void *vptr = physmap_phys_to_virt(p);
        return vptr ? vptr : (void *)(uintptr_t)p;
    }

    int num_units = (size + SLOB_UNIT - 1) / SLOB_UNIT + 1; // +1 for the header
    slob_block_t *prev = slob_free_list;
    slob_block_t *curr = prev->next;

    while (curr != NULL) {
        if (curr->units >= num_units) {
            if (curr->units == num_units) {
                prev->next = curr->next;
            } else {
                slob_block_t *split = (slob_block_t *)((char *)curr + num_units * SLOB_UNIT);
                split->units = curr->units - num_units;
                split->next = curr->next;
                prev->next = split;
                curr->units = num_units;
            }
            return (void *)(curr + 1); // Return space after the header
        }
        prev = curr;
        curr = curr->next;
    }

    // Allocate a new page if no suitable block found
    phys_addr_t paddr = mm_alloc_page(NUMA_NODE_ANY);
    if (!paddr) return NULL;

    slob_add_page(paddr);
    return kmalloc(size); // Retry allocation
}

void* kzalloc(size_t size) {
    void* ptr = kmalloc(size);
    if (ptr) {
        memset(ptr, 0, size);
    }
    return ptr;
}

void kfree(void* ptr) {
    if (!ptr) return;

    // Check if it's a page allocation
    phys_addr_t paddr = physmap_virt_to_phys(ptr);
    if (!paddr) paddr = (phys_addr_t)(uintptr_t)ptr;

    page_t *page = phys_to_page(paddr);
    if (page && (paddr % PAGE_SIZE == 0) && (uintptr_t)ptr == paddr) {
        // Heuristic to detect large allocations bypassing the SLOB
        bh_refcount_init(&page->ref_count, 1);
        mm_free_page(paddr);
        return;
    }

    // SLOB block free
    slob_block_t *b = (slob_block_t *)ptr - 1;

    // Insert into free list (LIFO style for simplicity)
    b->next = slob_free_list->next;
    slob_free_list->next = b;
    // Note: A real SLOB implementation would do coalescing here.
}

// Dummy kcache implementation since SLOB doesn't use slab caches directly
kcache_t* kcache_create(const char* name, size_t size) {
    kcache_t* c = (kcache_t*)kmalloc(sizeof(kcache_t));
    if (!c) return NULL;
    c->name = name;
    c->object_size = size;
    return c;
}

void* kcache_alloc(kcache_t* cache) {
    if (!cache) return NULL;
    return kmalloc(cache->object_size);
}

void kcache_free(kcache_t* cache, void* obj) {
    kfree(obj);
}

// Virtual Allocator (kvmalloc/kvfree)
typedef struct kvmalloc_node {
    virt_addr_t start;
    uint32_t pages;
    struct kvmalloc_node* next;
} kvmalloc_node_t;

static virt_addr_t next_kvmalloc_vaddr = 0xC0000000;
static kvmalloc_node_t* kvmalloc_head = NULL;

void* kvmalloc(size_t size) {
    if (size == 0) return NULL;
    uint32_t num_pages = (size + PAGE_SIZE - 1) / PAGE_SIZE;
    virt_addr_t start_vaddr = next_kvmalloc_vaddr;

    for (uint32_t i = 0; i < num_pages; i++) {
        phys_addr_t phys = mm_alloc_pages_order(0, NUMA_NODE_ANY, PAGE_FLAG_KERNEL);
        if (!phys) {
            return NULL;
        }
        mm_vmm_map_page(mm_create_address_space(), start_vaddr + (i * PAGE_SIZE), phys, PAGE_FLAG_KERNEL);
    }

    kvmalloc_node_t* node = (kvmalloc_node_t*)kmalloc(sizeof(kvmalloc_node_t));
    if (node) {
        node->start = start_vaddr;
        node->pages = num_pages;
        node->next = kvmalloc_head;
        kvmalloc_head = node;
    }

    next_kvmalloc_vaddr += (num_pages * PAGE_SIZE);
    return (void*)(uintptr_t)start_vaddr;
}

void kvfree(void* ptr) {
    if (!ptr) return;
    virt_addr_t vaddr = (virt_addr_t)(uintptr_t)ptr;

    kvmalloc_node_t** curr = &kvmalloc_head;
    while (*curr) {
        if ((*curr)->start == vaddr) {
            kvmalloc_node_t* to_free = *curr;
            for (uint32_t p = 0; p < to_free->pages; p++) {
                mm_vmm_unmap_page(mm_create_address_space(), vaddr + (p * PAGE_SIZE));
            }
            *curr = to_free->next;
            kfree(to_free);
            return;
        }
        curr = &(*curr)->next;
    }
}
