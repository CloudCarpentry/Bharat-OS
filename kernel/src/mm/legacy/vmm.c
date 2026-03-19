#include "../../include/mm.h"
#include "../../include/mm/aspace.h"
#include "../../include/hal/hal_pt.h"
#include "../../include/hal/hal_tlb.h"
#include "../../include/hal/hal.h"
#include "../../include/mm/pmm.h"

// M1: Legacy VMM is now just a thin shim over ASpace/Region/Object layers.

address_space_t kernel_space;

int vmm_init(void) {
    if (!active_hal_pt) {
        hal_pt_init();
    }

    // Initialize the kernel's address space wrapper
    kernel_space.object_id = 0;
    extern phys_addr_t vmm_get_kernel_root(void);
    kernel_space.root_pt = vmm_get_kernel_root();
    spin_lock_init(&kernel_space.lock);

    return 0;
}

address_space_t *mm_create_address_space(void) {
    address_space_t *as = NULL;
    aspace_create(&as, 0);
    return as;
}

phys_addr_t vmm_get_kernel_root(void) {
    if (active_hal_pt && active_hal_pt->create_address_space) {
        return active_hal_pt->create_address_space(0);
    }
    return 0;
}

int mm_vmm_map_page(address_space_t* as, virt_addr_t vaddr, phys_addr_t paddr, uint32_t flags) {
    if (!as || !active_hal_pt) return -1;

    // Look up authoritative region
    vm_region_t *region = aspace_lookup_region(as, vaddr);
    if (!region) {
        // Fallback for kernel direct mappings or legacy code without regions
        uint32_t mmu_flags = 0;
        if (flags & CAP_RIGHT_WRITE) mmu_flags |= HAL_PT_FLAG_WRITE;
        if (flags & PAGE_USER) mmu_flags |= HAL_PT_FLAG_USER;
        return active_hal_pt->map_page(as->root_pt, vaddr, paddr, mmu_flags);
    }

    // Normally we'd map via region->object, but this is a legacy compat shim.
    uint32_t mmu_flags = 0;
    if (flags & CAP_RIGHT_WRITE) mmu_flags |= HAL_PT_FLAG_WRITE;
    if (flags & PAGE_USER) mmu_flags |= HAL_PT_FLAG_USER;

    int ret = active_hal_pt->map_page(as->root_pt, vaddr, paddr, mmu_flags);
    if (ret == 0) {
        hal_tlb_invalidate_page(as, vaddr);
    }
    return ret;
}

int mm_vmm_unmap_page(address_space_t* as, virt_addr_t vaddr) {
    if (!as || !active_hal_pt) return -1;

    phys_addr_t paddr = 0;
    int ret = active_hal_pt->unmap_page(as->root_pt, vaddr, &paddr);
    if (ret == 0 && paddr != 0) {
        // Free the physical page (unless it's a device mapping or shared, but legacy shim assumes it's ours)
        mm_free_page(paddr);
        hal_tlb_invalidate_page(as, vaddr);
    }
    return ret;
}

int vmm_map_page(virt_addr_t vaddr, phys_addr_t paddr, uint32_t flags) {
    extern address_space_t kernel_space;
    return mm_vmm_map_page(&kernel_space, vaddr, paddr, flags);
}

int vmm_unmap_page(virt_addr_t vaddr) {
    extern address_space_t kernel_space;
    return mm_vmm_unmap_page(&kernel_space, vaddr);
}

int vmm_handle_cow_fault(address_space_t* as, virt_addr_t vaddr) {
    // Legacy shim simply routes back to the authoritative fault handler
    extern int vm_handle_fault(address_space_t* as, virt_addr_t fault_addr, uint32_t fault_flags);
    return vm_handle_fault(as, vaddr, CAP_RIGHT_WRITE);
}

void tlb_shootdown(address_space_t *as, virt_addr_t vaddr) {
    hal_tlb_invalidate_page(as, vaddr);
}

// Dummy functions for compilation. They should ideally be part of HAL or device drivers.
int vmm_map_device_mmio(virt_addr_t vaddr, phys_addr_t paddr, capability_t *cap, int is_npu) {
    (void)cap;
    (void)is_npu;
    return vmm_map_page(vaddr, paddr, CAP_RIGHT_WRITE);
}

int vmm_map_device_mmio_token(virt_addr_t vaddr, phys_addr_t paddr,
                              uint64_t size, const bharat_addr_token_t *token,
                              int is_npu) {
    (void)size;
    (void)token;
    (void)is_npu;
    return vmm_map_page(vaddr, paddr, CAP_RIGHT_WRITE);
}

void vmm_process_local_urpc_messages(uint32_t core_id) {
    (void)core_id;
    extern void vmm_process_urpc_messages(void);
    vmm_process_urpc_messages();
}
