#include "../../include/mm.h"
#include "../../include/numa.h"
#include "../../include/atomic.h"
#include "../../include/profile.h"
#include "../../include/spinlock.h"
#include <stddef.h>
#include <stdint.h>

/*
 * Bharat-OS Physical Memory Allocator
 * Production-Grade Buddy Allocator with NUMA-awareness and lock safety.
 */

#define MAX_ORDER 11
#define MAX_NUMA_NODES 4
#define MAX_PHYS_PAGES 1048576 // Supports up to 4GB of physical RAM initially
#define MAX_PAGES_PER_NODE (MAX_PHYS_PAGES / MAX_NUMA_NODES)

// The global page array for the entire system
static page_t global_pages[MAX_PHYS_PAGES];

typedef struct {
    spinlock_t lock;
    list_head_t free_list[MAX_ORDER];
    size_t free_count[MAX_ORDER];
} zone_t;

static zone_t numa_zones[MAX_NUMA_NODES];
static numa_node_t numa_nodes[MAX_NUMA_NODES];
static uint32_t active_numa_nodes = 0;

phys_addr_t page_to_phys(page_t *page) {
    size_t global_index = page - global_pages;
    // Determine which NUMA node this page belongs to
    uint32_t node_id = global_index / MAX_PAGES_PER_NODE;
    size_t node_index = global_index % MAX_PAGES_PER_NODE;
    return numa_nodes[node_id].start_addr + (node_index * PAGE_SIZE);
}

page_t *phys_to_page(phys_addr_t phys) {
    for (uint32_t i = 0; i < active_numa_nodes; i++) {
        numa_node_t *node = &numa_nodes[i];
        if (phys >= node->start_addr && phys < node->start_addr + (node->total_pages * PAGE_SIZE)) {
            size_t node_index = (phys - node->start_addr) / PAGE_SIZE;
            size_t global_index = (i * MAX_PAGES_PER_NODE) + node_index;
            return &global_pages[global_index];
        }
    }
    return NULL;
}

// Internal helper for buddy allocation
static void* pmm_alloc_pages_order(int order, int numa_node) {
    zone_t *zone = &numa_zones[numa_node];

    spin_lock(&zone->lock); // Ensure thread safety for SMP

    for (int i = order; i < MAX_ORDER; i++) {
        if (!list_empty(&zone->free_list[i])) {
            page_t *page = list_entry(zone->free_list[i].next, page_t, list);
            list_del(&page->list);
            zone->free_count[i]--;

            // Split larger blocks (Buddy System)
            while (i > order) {
                i--;
                page_t *buddy = page + (1 << i);
                buddy->order = i;
                list_add(&buddy->list, &zone->free_list[i]);
                zone->free_count[i]++;
            }

            page->order = order;
            page->ref_count = 1;
            spin_unlock(&zone->lock);
            return (void*)page_to_phys(page);
        }
    }

    spin_unlock(&zone->lock);
    return NULL; // Out of memory
}

int mm_pmm_init(void* memory_map, uint32_t map_size) {
    // Bootstrap Phase: Start with a single "Default Node"
    active_numa_nodes = 1;

    for (uint32_t i = 0; i < active_numa_nodes; i++) {
        numa_nodes[i].node_id = i;
        numa_nodes[i].start_addr = i * (MAX_PAGES_PER_NODE * PAGE_SIZE);
        numa_nodes[i].total_pages = MAX_PAGES_PER_NODE;
        numa_nodes[i].free_pages = 0;
        numa_nodes[i].allocator_metadata = &numa_zones[i];

        zone_t *zone = &numa_zones[i];
        spin_lock_init(&zone->lock);
        for (int o = 0; o < MAX_ORDER; o++) {
            list_init(&zone->free_list[o]);
            zone->free_count[o] = 0;
        }

        // Initialize global_pages for this node as used initially
        for (size_t j = 0; j < MAX_PAGES_PER_NODE; j++) {
            size_t global_index = (i * MAX_PAGES_PER_NODE) + j;
            global_pages[global_index].ref_count = 1; // used
            global_pages[global_index].numa_node = i;
            global_pages[global_index].flags = 0;
            global_pages[global_index].order = -1;
            list_init(&global_pages[global_index].list);
        }
    }
    
    // Simulate freeing memory: pages 1 through (MAX_PAGES_PER_NODE/2) on each node
    for (uint32_t i = 0; i < active_numa_nodes; i++) {
        for (size_t j = 1; j < (MAX_PAGES_PER_NODE / 2); j++) {
            size_t global_index = (i * MAX_PAGES_PER_NODE) + j;
            phys_addr_t phys = numa_nodes[i].start_addr + (j * PAGE_SIZE);
            // Freeing individual pages will populate the buddy system
            mm_free_page(phys);
        }
    }

    return 0; // Success
}

phys_addr_t mm_alloc_page(uint32_t preferred_numa_node) {
    if (preferred_numa_node >= active_numa_nodes && preferred_numa_node != NUMA_NODE_ANY) {
        preferred_numa_node = 0; // Fallback
    }

    // Attempt allocation from the local node first
    uint32_t start_node = (preferred_numa_node == NUMA_NODE_ANY) ? numa_get_current_node() : preferred_numa_node;
    if (start_node >= active_numa_nodes) start_node = 0;

    for (uint32_t attempt = 0; attempt < active_numa_nodes; attempt++) {
        uint32_t node_id = (start_node + attempt) % active_numa_nodes;
        void* addr = pmm_alloc_pages_order(0, node_id);
        if (addr != NULL) {
            atomic64_fetch_and_sub_ptr(&numa_nodes[node_id].free_pages, 1);
            return (phys_addr_t)addr;
        }
    }
    
    return 0; // Out of memory
}

void mm_free_page(phys_addr_t page_addr) {
    page_t *page = phys_to_page(page_addr);
    if (!page) return;

    // Decrement ref count first
    uint16_t old_ref = atomic16_fetch_and_sub(&page->ref_count, 1);
    if (old_ref != 1) {
        return; // Still in use or double free
    }

    uint32_t node_id = page->numa_node;
    zone_t *zone = &numa_zones[node_id];
    size_t node_index = (page_addr - numa_nodes[node_id].start_addr) / PAGE_SIZE;
    int order = page->order;
    if (order < 0) order = 0; // If freed directly via init without an order

    int original_order = order;

    spin_lock(&zone->lock);

    // Coalescing logic
    while (order < MAX_ORDER - 1) {
        size_t buddy_index = node_index ^ (1ULL << order);
        size_t buddy_global_index = (node_id * MAX_PAGES_PER_NODE) + buddy_index;
        page_t *buddy = &global_pages[buddy_global_index];

        // If buddy is not free or not same order, stop coalescing
        if (buddy->ref_count > 0 || buddy->order != order) {
            break;
        }

        // Remove buddy from free list
        list_del(&buddy->list);
        zone->free_count[order]--;

        // Combine buddies
        if (buddy_index < node_index) {
            page = buddy;
            node_index = buddy_index;
        }
        order++;
    }

    page->order = order;
    page->ref_count = 0;
    list_add(&page->list, &zone->free_list[order]);
    zone->free_count[order]++;

    spin_unlock(&zone->lock);
    atomic64_fetch_and_add_ptr(&numa_nodes[node_id].free_pages, 1ULL << original_order);
}

// Used to support Copy-on-Write (CoW)
#ifndef Profile_RTOS
void mm_inc_page_ref(phys_addr_t page_addr) {
    page_t *page = phys_to_page(page_addr);
    if (page && page->ref_count > 0) {
        atomic16_fetch_and_add(&page->ref_count, 1);
    }
}
#else
void mm_inc_page_ref(phys_addr_t page_addr) {
    // CoW is disabled in RTOS profiles for deterministic performance
}
#endif
