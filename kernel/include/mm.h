#ifndef BHARAT_MM_H
#define BHARAT_MM_H

#include <stdint.h>

/*
 * Bharat-OS Memory Management Subsystem
 * Supports NUMA-awareness for datacenter scalability and standard paging.
 */

// Basic page definitions (Assuming 4KB base pages)
#define PAGE_SIZE 4096

typedef uint64_t phys_addr_t;
typedef uint64_t virt_addr_t;

// NUMA Node Definition
typedef struct {
    uint32_t node_id;
    phys_addr_t start_addr;
    uint64_t total_pages;
    uint64_t free_pages;
    // Implementation specific bitmap or buddy allocator metadata here
    void* allocator_metadata;
} numa_node_t;

// Initialize physical memory allocator natively using Multiboot/SBI memory maps
int mm_pmm_init(void* memory_map, uint32_t map_size);

// Base Page Allocation (NUMA aware)
phys_addr_t mm_alloc_page(uint32_t preferred_numa_node);
void mm_free_page(phys_addr_t page);

// Virtual Memory Management (Architecture agnostic paging)
typedef struct {
    phys_addr_t root_table; // CR3 on x86, satp on RISC-V
} address_space_t;

int mm_vmm_map_page(address_space_t* as, virt_addr_t vaddr, phys_addr_t paddr, uint32_t flags);
int mm_vmm_unmap_page(address_space_t* as, virt_addr_t vaddr);

// Create a new empty hardware address space
address_space_t* mm_create_address_space(void);

#endif // BHARAT_MM_H
