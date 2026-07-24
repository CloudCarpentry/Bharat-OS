#include "../../include/slab.h"
#include "../../include/mm.h"
#include "../../include/numa.h"
#include "../../include/mm/physmap.h"
#include <stddef.h>
#include "lib/base/string.h"

#define NUM_SLAB_SIZES 7

static size_t slab_sizes[NUM_SLAB_SIZES] = { 32, 64, 128, 256, 512, 1024, 2048 };
static kcache_t slab_caches[NUM_SLAB_SIZES];
static int slab_initialized = 0;

static void init_slab() {
    if (slab_initialized) return;
    for (int i = 0; i < NUM_SLAB_SIZES; i++) {
        slab_caches[i].name = "kmalloc_slab";
        slab_caches[i].object_size = slab_sizes[i];
        slab_caches[i].num_pages = 0;
        slab_caches[i].objs_per_page = PAGE_SIZE / slab_caches[i].object_size;
        slab_caches[i].bitmap_words_per_page = (slab_caches[i].objs_per_page + 31u) / 32u;
        for (int j = 0; j < KCACHE_MAX_PAGES; j++) {
            for (int w = 0; w < KCACHE_BITMAP_WORDS_PER_PAGE; w++) {
                slab_caches[i].free_bitmap[j][w] = 0;
            }
        }
    }
    slab_initialized = 1;
}

// Custom kcache implementation
kcache_t* kcache_create(const char* name, size_t size) {
    // Force minimum 16-byte alignment
    if (size < SLAB_MIN_OBJECT_SIZE) {
        size = SLAB_MIN_OBJECT_SIZE;
    } else {
        size = (size + 15) & ~15;
    }

    kcache_t* c = (kcache_t*)kmalloc(sizeof(kcache_t));
    if (!c) return NULL;
    c->name = name;
    c->object_size = size;
    c->num_pages = 0;
    c->objs_per_page = PAGE_SIZE / size;
    c->bitmap_words_per_page = (c->objs_per_page + 31u) / 32u;
    for (int j = 0; j < KCACHE_MAX_PAGES; j++) {
        for (int w = 0; w < KCACHE_BITMAP_WORDS_PER_PAGE; w++) {
            c->free_bitmap[j][w] = 0;
        }
    }
    return c;
}

void* kcache_alloc(kcache_t* cache) {
    if (!cache || cache->object_size == 0) return NULL;
    uint32_t objs_per_page = cache->objs_per_page;

    // Find free object
    for (uint32_t p = 0; p < cache->num_pages; p++) {
        for (uint32_t i = 0; i < objs_per_page; i++) {
            uint32_t word = i / 32u;
            uint32_t bit = i % 32u;
            if ((cache->free_bitmap[p][word] & (1U << bit)) == 0) {
                cache->free_bitmap[p][word] |= (1U << bit);
                phys_addr_t paddr = cache->pages[p] + (i * cache->object_size);
                return physmap_phys_to_virt(paddr);
            }
        }
    }

    // Allocate new page
    if (cache->num_pages >= KCACHE_MAX_PAGES) return NULL; // simple limitation
    phys_addr_t page = mm_alloc_page(NUMA_NODE_ANY);
    if (!page) return NULL;

    uint32_t p = cache->num_pages++;
    cache->pages[p] = page;

    for (int w = 0; w < KCACHE_BITMAP_WORDS_PER_PAGE; w++) {
        cache->free_bitmap[p][w] = 0;
    }

    cache->free_bitmap[p][0] = (1U << 0); // Allocate first object
    return physmap_phys_to_virt(page);
}

void kcache_free(kcache_t* cache, void* obj) {
    if (!cache || !obj) return;
    phys_addr_t paddr = physmap_virt_to_phys(obj);

    for (uint32_t p = 0; p < cache->num_pages; p++) {
        if (paddr >= cache->pages[p] && paddr < cache->pages[p] + PAGE_SIZE) {
            uint32_t i = (paddr - cache->pages[p]) / cache->object_size;
            if (i < cache->objs_per_page) {
                uint32_t word = i / 32u;
                uint32_t bit = i % 32u;
                cache->free_bitmap[p][word] &= ~(1U << bit);
            }
            return;
        }
    }
}

void* kmalloc(size_t size) {
    if (!slab_initialized) init_slab();

    if (size > 2048) {
        // Fallback to page allocation
        int order = 0;
        size_t s = PAGE_SIZE;
        while (s < size) {
            order++;
            s *= 2;
        }
        phys_addr_t p = mm_alloc_pages_order(order, NUMA_NODE_ANY, PAGE_FLAG_KERNEL);
        if (!p) return NULL;
        return physmap_phys_to_virt(p);
    }

    for (int i = 0; i < NUM_SLAB_SIZES; i++) {
        if (size <= slab_sizes[i]) {
            return kcache_alloc(&slab_caches[i]);
        }
    }
    return NULL;
}

void* kzalloc(size_t size) {
    void* ptr = kmalloc(size);
    if (ptr) {
        // memset expects a virtual address
        memset(ptr, 0, size);
    }
    return ptr;
}

void kfree(void* ptr) {
    if (!ptr) return;
    phys_addr_t paddr = physmap_virt_to_phys(ptr);

    // Check if it's in slab
    for (int i = 0; i < NUM_SLAB_SIZES; i++) {
        for (uint32_t p = 0; p < slab_caches[i].num_pages; p++) {
            if (paddr >= slab_caches[i].pages[p] && paddr < slab_caches[i].pages[p] + PAGE_SIZE) {
                kcache_free(&slab_caches[i], ptr);
                return;
            }
        }
    }

    // Otherwise it's a direct page allocation
    page_t *page = phys_to_page(paddr);
    if (page) {
        bh_refcount_init(&page->ref_count, 1); // Prepare for mm_free_page
        mm_free_page(paddr);
    }
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
