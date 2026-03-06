#include "../../include/mm.h"
#include "../../include/numa.h"
#include <stddef.h>

/*
 * Bharat-OS Physical Memory Allocator
 * Uses a basic bitmap to track allocated physical pages (4KB).
 * Supports NUMA-node awareness, spinlocks for thread safety, and reference counting for Copy-on-Write (CoW).
 */

// Assuming PAGE_SIZE is defined in mm.h (4096)
#define MAX_NUMA_NODES 4
#define MAX_PHYS_PAGES 1048576 // Supports up to 4GB of physical RAM initially
#define MAX_PAGES_PER_NODE (MAX_PHYS_PAGES / MAX_NUMA_NODES)

static numa_node_t numa_nodes[MAX_NUMA_NODES];
static uint32_t active_numa_nodes = 0;

typedef struct {
    int spinlock;
    uint8_t bitmap[MAX_PAGES_PER_NODE / 8];
    uint16_t ref_counts[MAX_PAGES_PER_NODE];
} pmm_node_data_t;

static pmm_node_data_t node_data[MAX_NUMA_NODES];

// Atomic spinlock implementation
static inline void spin_lock(int* lock) {
    while (__sync_lock_test_and_set(lock, 1)) {
        // busy wait
    }
}

static inline void spin_unlock(int* lock) {
    __sync_lock_release(lock);
}

// Internal helper to set a bit in the bitmap
static inline void set_page_used(pmm_node_data_t* data, size_t index) {
    data->bitmap[index / 8] |= (1 << (index % 8));
}

// Internal helper to clear a bit in the bitmap
static inline void clear_page_used(pmm_node_data_t* data, size_t index) {
    data->bitmap[index / 8] &= ~(1 << (index % 8));
}

// Internal helper to check a bit
static inline int is_page_used(pmm_node_data_t* data, size_t index) {
    return (data->bitmap[index / 8] & (1 << (index % 8))) != 0;
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

        node_data[i].spinlock = 0;

        // Safety: Mark ALL pages as used initially to prevent allocating unavailable/reserved memory
        for (size_t j = 0; j < (MAX_PAGES_PER_NODE / 8); j++) {
            node_data[i].bitmap[j] = 0xFF;
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

        spin_lock(&data->spinlock);
        for (size_t j = 1; j < (MAX_PAGES_PER_NODE / 2); j++) {
            clear_page_used(data, j);
            data->ref_counts[j] = 0;
            node->free_pages++;
        }
        spin_unlock(&data->spinlock);
    }

    return 0; // Success
}

phys_addr_t mm_alloc_page(uint32_t preferred_numa_node) {
    if (preferred_numa_node >= active_numa_nodes && preferred_numa_node != NUMA_NODE_ANY) {
        preferred_numa_node = 0; // Fallback
    }

    uint32_t start_node = (preferred_numa_node == NUMA_NODE_ANY) ? 0 : preferred_numa_node;

    for (uint32_t attempt = 0; attempt < active_numa_nodes; attempt++) {
        uint32_t node_id = (start_node + attempt) % active_numa_nodes;
        numa_node_t* node = &numa_nodes[node_id];
        pmm_node_data_t* data = (pmm_node_data_t*)node->allocator_metadata;

        spin_lock(&data->spinlock);
        if (node->free_pages > 0) {
            for (size_t i = 0; i < node->total_pages; i++) {
                if (!is_page_used(data, i)) {
                    set_page_used(data, i);
                    data->ref_counts[i] = 1;
                    node->free_pages--;
                    spin_unlock(&data->spinlock);
                    return node->start_addr + (i * PAGE_SIZE);
                }
            }
        }
        spin_unlock(&data->spinlock);
    }
    
    return 0; // Out of memory
}

void mm_free_page(phys_addr_t page) {
    for (uint32_t i = 0; i < active_numa_nodes; i++) {
        numa_node_t* node = &numa_nodes[i];
        if (page >= node->start_addr && page < node->start_addr + (node->total_pages * PAGE_SIZE)) {
            size_t index = (page - node->start_addr) / PAGE_SIZE;
            pmm_node_data_t* data = (pmm_node_data_t*)node->allocator_metadata;

            spin_lock(&data->spinlock);
            if (is_page_used(data, index)) {
                if (data->ref_counts[index] > 0) {
                    data->ref_counts[index]--;
                }

                if (data->ref_counts[index] == 0) {
                    clear_page_used(data, index);
                    node->free_pages++;
                }
            }
            spin_unlock(&data->spinlock);
            return;
        }
    }
}

// Used to support Copy-on-Write (CoW)
void mm_inc_page_ref(phys_addr_t page) {
    for (uint32_t i = 0; i < active_numa_nodes; i++) {
        numa_node_t* node = &numa_nodes[i];
        if (page >= node->start_addr && page < node->start_addr + (node->total_pages * PAGE_SIZE)) {
            size_t index = (page - node->start_addr) / PAGE_SIZE;
            pmm_node_data_t* data = (pmm_node_data_t*)node->allocator_metadata;

            spin_lock(&data->spinlock);
            if (is_page_used(data, index)) {
                data->ref_counts[index]++;
            }
            spin_unlock(&data->spinlock);
            return;
        }
    }
}
