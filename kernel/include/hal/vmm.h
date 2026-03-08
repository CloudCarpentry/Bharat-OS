#ifndef BHARAT_HAL_VMM_H
#define BHARAT_HAL_VMM_H

#include <stdint.h>
#include "../../include/mm.h"

// Initialize the root page table (e.g., PML4 for x86_64, SATP for RISC-V, TTBR for ARM64)
// This function should allocate a root table and return its physical address, or 0 on failure.
phys_addr_t hal_vmm_init_root(void);

// Map a single page into the given root page table
int hal_vmm_map_page(phys_addr_t root_table, virt_addr_t vaddr, phys_addr_t paddr, uint32_t flags);

// Unmap a single page from the given root page table
int hal_vmm_unmap_page(phys_addr_t root_table, virt_addr_t vaddr, phys_addr_t* unmapped_paddr);

// Retrieve the physical address and flags mapped at a given virtual address
int hal_vmm_get_mapping(phys_addr_t root_table, virt_addr_t vaddr, phys_addr_t* paddr, uint32_t* flags);

// Setup a new address space by copying kernel mappings from the kernel root table
phys_addr_t hal_vmm_setup_address_space(phys_addr_t kernel_root_table);

// Update mapping flags (e.g., for CoW faults where we just update the PTE)
int hal_vmm_update_mapping(phys_addr_t root_table, virt_addr_t vaddr, phys_addr_t paddr, uint32_t flags);

#endif // BHARAT_HAL_VMM_H
