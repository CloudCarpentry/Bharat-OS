#include "../../include/mm.h"
#include "../../include/mm/aspace.h"
#include "../../include/mm/vm_object.h"
#include "../../include/mm/pmm.h"
#include "../../include/hal/hal_pt.h"
#include "../../include/hal/hal_tlb.h"
#include "../../include/kernel.h"
#include "../../include/hal/mmu_ops.h"

// VM Fault Engine: Core page fault dispatcher and handler

int vm_handle_fault(address_space_t *aspace, virt_addr_t fault_addr, uint32_t fault_flags) {
    if (!aspace || !active_hal_pt) {
        return -1;
    }

    // 1. Authoritative lookup: Find the region mapping this VA
    vm_region_t *region = aspace_lookup_region(aspace, fault_addr);
    if (!region) {
        // Unmapped address, check for kernel fallback/direct mappings
        return -1; // Segment fault
    }

    // 2. Validate access rights
    if ((fault_flags & CAP_RIGHT_WRITE) && !(region->prot & CAP_RIGHT_WRITE)) {
        return -2; // Permission fault
    }
    if ((fault_flags & CAP_RIGHT_EXECUTE) && !(region->prot & CAP_RIGHT_EXECUTE)) {
        return -2; // Execute permission fault
    }

    // 3. Delegate to the underlying object backend
    vm_object_t *object = region->object;
    if (!object || !object->ops || !object->ops->fault) {
        return -3; // No backing object or no fault handler
    }

    phys_addr_t paddr = 0;
    uint32_t page_flags = 0;

    int res = object->ops->fault(object, region, fault_addr, fault_flags, &paddr, &page_flags);
    if (res != VM_FAULT_HANDLED) {
        return res;
    }

    // Map it in the page table
    uint32_t mmu_flags = MMU_USER;
    if (page_flags & CAP_RIGHT_WRITE) mmu_flags |= MMU_WRITE;
    if (page_flags & CAP_RIGHT_EXECUTE) mmu_flags |= MMU_EXEC;

    virt_addr_t aligned_vaddr = fault_addr & ~(PAGE_SIZE - 1U);
    int ret = active_hal_pt->map_page(aspace->root_pt, aligned_vaddr, paddr, mmu_flags);
    if (ret == 0) {
        hal_tlb_invalidate_page(aspace, aligned_vaddr);
    }
    return ret;
}
