#include "../../../include/mm.h"
#include "../../../include/mm/aspace.h"
#include "../../../include/hal/hal_pt.h"
#include "../../../include/hal/mmu_ops.h"
#include "../../../include/hal/hal_tlb.h"
#include "../../../include/mm/pmm.h"
#include "../../../include/slab.h"

// M1: Legacy VMM is now just a thin shim over ASpace/Region/Object layers.

void vmm_process_urpc_messages(void);

int mm_vmm_map_page(address_space_t* as, virt_addr_t vaddr, phys_addr_t paddr, uint32_t flags) {
    if (!as || !active_hal_pt) return -1;

    // Look up authoritative region
    vm_region_t *region = aspace_lookup_region(as, vaddr);
    if (!region) {
        // Fallback for kernel direct mappings or legacy code without regions
        mmu_flags_t mmu_flags = 0;
        if (flags & CAP_RIGHT_WRITE) mmu_flags |= MMU_WRITE;
        if (flags & PAGE_USER) mmu_flags |= MMU_USER;
        return active_hal_pt->map_page(as->root_pt, vaddr, paddr, mmu_flags);
    }

    // Normally we'd map via region->object, but this is a legacy compat shim.
    mmu_flags_t mmu_flags = 0;
    if (flags & CAP_RIGHT_WRITE) mmu_flags |= MMU_WRITE;
    if (flags & PAGE_USER) mmu_flags |= MMU_USER;

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

address_space_t kernel_space;
static int kernel_space_ready = 0;
static phys_addr_t kernel_root_pt = 0;

static void ensure_kernel_space_ready(void) {
    if (kernel_space_ready) return;

    address_space_t *created = NULL;
    if (aspace_create(&created, 0) == 0 && created) {
        kernel_space = *created;
        kfree(created);
        kernel_root_pt = kernel_space.root_pt;
        kernel_space_ready = 1;
    }
}

int vmm_init(void) {
    ensure_kernel_space_ready();
    return kernel_space_ready ? 0 : -1;
}

phys_addr_t vmm_get_kernel_root(void) {
    ensure_kernel_space_ready();
    return kernel_root_pt;
}

address_space_t *mm_create_address_space(void) {
    address_space_t *as = NULL;
    if (aspace_create(&as, 0) != 0) return NULL;
    return as;
}

void vmm_process_local_urpc_messages(uint32_t core_id) {
    (void)core_id;
    vmm_process_urpc_messages();
}

int vmm_map_device_mmio(virt_addr_t vaddr, phys_addr_t paddr, capability_t *cap, int is_npu) {
    (void)is_npu;
    if (!cap) return -1;
    return vmm_map_page(vaddr, paddr, cap->rights_mask);
}
