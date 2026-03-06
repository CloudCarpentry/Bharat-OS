#include "../../include/mm.h"
#include "../../include/numa.h"
#include "../../include/hal/hal.h"
#include "../../include/advanced/formal_verif.h"
#include <stddef.h>

/*
 * Bharat-OS Virtual Memory Manager
 * Handles architecture-agnostic Demand Paging structures and capabilities.
 * Hardware-specific paging (e.g., x86 Page Directories or ARM translation tables) 
 * will be linked in via the HAL during compilation.
 */

// Global kernel page directory structure (architecture abstraction)
static void* kernel_page_directory;

int vmm_init() {
    // 1. Ask the physical memory manager for a single 4KB page to hold the Root Directory
    phys_addr_t root_dir_phys = mm_alloc_page(NUMA_NODE_ANY);
    
    if (root_dir_phys == 0) {
        return -1; // Failed to allocate root page directory
    }
    
    kernel_page_directory = (void*)(uintptr_t)root_dir_phys; // Direct physical mapping for early boot
    
    // Future: In a full system, invoke HAL to set architecture-specific CR3 (x86) or TTBR0 (ARM) registers
    
    return 0;
}

int vmm_map_page(virt_addr_t vaddr, phys_addr_t paddr, uint32_t flags) {
    if (!kernel_page_directory) return -1;
    
    // Stub: This relies entirely on hardware architecture specifics.
    // E.g., for x86_64 this would resolve the PML4 -> PDPT -> PD -> PT layers.
    
    return 0; // Success stub
}

int vmm_map_device_mmio(virt_addr_t vaddr, phys_addr_t paddr, capability_t* cap, int is_npu) {
    if (!cap) return -1;

    // Enforce capability gating for AI Hardware
    uint32_t required_right = is_npu ? CAP_RIGHT_DEVICE_NPU : CAP_RIGHT_DEVICE_GPU;
    if ((cap->rights_mask & required_right) == 0) {
        return -2; // Unauthorized access
    }

    return vmm_map_page(vaddr, paddr, CAP_RIGHT_READ | CAP_RIGHT_WRITE);
}

int vmm_unmap_page(virt_addr_t vaddr) {
    if (!kernel_page_directory) return -1;
    
    // Stub: Remove the hardware mapping and translation cache (TLB) flush
    
    // TLB Coherency: Ensure hardware removes the cached translation
    hal_tlb_flush(vaddr);

    return 0; // Success stub
}
