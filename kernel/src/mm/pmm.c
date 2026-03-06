#include "../../include/mm.h"
#include "../../include/numa.h"
#include <stddef.h>
#include <stdint.h>

/*
 * Bharat-OS Physical Memory Allocator
 * Uses a basic bitmap to track allocated physical pages (4KB).
 * Supports NUMA-node awareness, lock-free atomics for thread safety, and reference counting for Copy-on-Write (CoW).
 */

// Assuming PAGE_SIZE is defined in mm.h (4096)
#define MAX_NUMA_NODES 4
#define MAX_PHYS_PAGES 1048576 // Supports up to 4GB of physical RAM initially
#define MAX_PAGES_PER_NODE (MAX_PHYS_PAGES / MAX_NUMA_NODES)

static numa_node_t numa_nodes[MAX_NUMA_NODES];
static uint32_t active_numa_nodes = 0;

typedef struct {
    uint64_t bitmap[MAX_PAGES_PER_NODE / 64];
    uint16_t ref_counts[MAX_PAGES_PER_NODE];
} pmm_node_data_t;

static pmm_node_data_t node_data[MAX_NUMA_NODES];

// Internal helper to clear a bit in the bitmap
static inline void clear_page_used(pmm_node_data_t* data, size_t index) {
    uint64_t mask = 1ULL << (index % 64);
    __sync_fetch_and_and(&data->bitmap[index / 64], ~mask);
}

// Internal helper to check a bit
static inline int is_page_used(pmm_node_data_t* data, size_t index) {
    return (data->bitmap[index / 64] & (1ULL << (index % 64))) != 0;
}

int mm_pmm_init(void* memory_map, uint32_t map_size) {
    // Basic mock of NUMA nodes. In a real system, we'd parse ACPI SRAT.
    active_numa_nodes = 2; // Assume 2 nodes for demonstration

    for (uint32_t i = 0; i < active_numa_nodes; i++) {
        numa_nodes[i].node_id = i;
        numa_nodes[i].start_addr = i * (MAX_PAGES_PER_NODE * PAGE_SIZE);
        numa_nodes[i].total_pages = MAX_PAGES_PER_NODE;
        numa_nodes[i].free_pages = 0; // Start with 0 free pages until explicitly freed
        numa_nodes[i].allocator_metadata = &node_data[i];

        // Safety: Mark ALL pages as used initially to prevent allocating unavailable/reserved memory
        for (size_t j = 0; j < (MAX_PAGES_PER_NODE / 64); j++) {
            node_data[i].bitmap[j] = 0xFFFFFFFFFFFFFFFFULL;
        }
        for (size_t j = 0; j < MAX_PAGES_PER_NODE; j++) {
            node_data[i].ref_counts[j] = 1; // Mark as referenced to represent "used" state
        }
    }
    
    // In a real implementation, we would parse memory_map here.
    // For this demonstration, we simulate freeing available memory based on the bootloader map.
    // We assume pages 1 through (MAX_PAGES_PER_NODE/2) on each node are free RAM.
    for (uint32_t i = 0; i < active_numa_nodes; i++) {
        numa_node_t* node = &numa_nodes[i];
        pmm_node_data_t* data = (pmm_node_data_t*)node->allocator_metadata;

        for (size_t j = 1; j < (MAX_PAGES_PER_NODE / 2); j++) {
            clear_page_used(data, j);
            data->ref_counts[j] = 0;
            __sync_fetch_and_add(&node->free_pages, 1);
        }
    }

    return 0; // Success
}

phys_addr_t mm_alloc_page(uint32_t preferred_numa_node) {
    if (preferred_numa_node >= active_numa_nodes && preferred_numa_node != NUMA_NODE_ANY) {
        preferred_numa_node = 0; // Fallback
    }

    // Attempt allocation from the local node first (NUMA aware scaling)
    uint32_t start_node = (preferred_numa_node == NUMA_NODE_ANY) ? numa_get_current_node() : preferred_numa_node;
    if (start_node >= active_numa_nodes) start_node = 0;

    for (uint32_t attempt = 0; attempt < active_numa_nodes; attempt++) {
        uint32_t node_id = (start_node + attempt) % active_numa_nodes;
        numa_node_t* node = &numa_nodes[node_id];
        pmm_node_data_t* data = (pmm_node_data_t*)node->allocator_metadata;

        if (node->free_pages > 0) {
            size_t num_elements = MAX_PAGES_PER_NODE / 64;
            for (size_t i = 0; i < num_elements; i++) {
                uint64_t current = data->bitmap[i];
                if (current != 0xFFFFFFFFFFFFFFFFULL) { // Found a 64-bit block with at least one 0 (free page)
                    int bit_idx = __builtin_ctzll(~current); // Count trailing zeros of the inverted block
                    uint64_t mask = 1ULL << bit_idx;

                    // Atomically set the bit. If the previous state didn't have the bit set, we successfully claimed it.
                    if (!(__sync_fetch_and_or(&data->bitmap[i], mask) & mask)) {
                        size_t page_index = (i * 64) + bit_idx;
                        data->ref_counts[page_index] = 1;
                        __sync_fetch_and_sub(&node->free_pages, 1);
                        return node->start_addr + (page_index * PAGE_SIZE);
                    }
                }
            }
        }
    }
    
    return 0; // Out of memory
}

void mm_free_page(phys_addr_t page) {
    for (uint32_t i = 0; i < active_numa_nodes; i++) {
        numa_node_t* node = &numa_nodes[i];
        if (page >= node->start_addr && page < node->start_addr + (node->total_pages * PAGE_SIZE)) {
            size_t index = (page - node->start_addr) / PAGE_SIZE;
            pmm_node_data_t* data = (pmm_node_data_t*)node->allocator_metadata;

            if (is_page_used(data, index)) {
                uint16_t old_ref = __sync_fetch_and_sub(&data->ref_counts[index], 1);

                if (old_ref == 1) { // It dropped to 0
                    clear_page_used(data, index);
                    __sync_fetch_and_add(&node->free_pages, 1);
                }
            }
            return;
        }
    }
}

// Used to support Copy-on-Write (CoW)
#ifndef Profile_RTOS
void mm_inc_page_ref(phys_addr_t page) {
    for (uint32_t i = 0; i < active_numa_nodes; i++) {
        numa_node_t* node = &numa_nodes[i];
        if (page >= node->start_addr && page < node->start_addr + (node->total_pages * PAGE_SIZE)) {
            size_t index = (page - node->start_addr) / PAGE_SIZE;
            pmm_node_data_t* data = (pmm_node_data_t*)node->allocator_metadata;

            if (is_page_used(data, index)) {
                __sync_fetch_and_add(&data->ref_counts[index], 1);
            }
            return;
        }
    }
}
#else
void mm_inc_page_ref(phys_addr_t page) {
    // CoW is disabled in RTOS profiles for deterministic performance
}
#endif
