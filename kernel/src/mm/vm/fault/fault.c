#include "../../include/mm.h"
#include "../../include/mm/aspace.h"
#include "../../include/mm/vm_object.h"
#include "../../include/mm/pmm.h"
#include "../../include/hal/hal_pt.h"
#include "../../include/hal/hal_tlb.h"
#include "../../include/kernel.h"

// VM Fault Engine: Core page fault dispatcher and handler

int vm_handle_fault(address_space_t *aspace, virt_addr_t fault_addr, uint32_t fault_flags) {
    if (!aspace || !active_hal_pt) {
        return VM_FAULT_SIGSEGV;
    }

    vm_region_t *region = aspace_lookup_region(aspace, fault_addr);
    if (!region) {
        return VM_FAULT_SIGSEGV;
    }

    // 2. Validate access rights
    if ((fault_flags & CAP_RIGHT_WRITE) && !(region->prot & CAP_RIGHT_WRITE)) {
        return -2; // Permission fault
    }
    if ((fault_flags & CAP_RIGHT_EXECUTE) && !(region->prot & CAP_RIGHT_EXECUTE)) {
        return -2; // Execute permission fault
    }

    vm_object_t *object = region->object;
    if (!object || !object->ops || !object->ops->fault) {
        return VM_FAULT_SIGBUS;
    }

    // Calculate offset within the object
    uint64_t offset_in_region = fault_addr - region->base;
    uint64_t object_offset = region->object_offset + offset_in_region;

    // Delegate to object backend (Anon/Shared/File/Device)
    phys_addr_t out_page = 0;
    uint32_t out_flags = 0;
    int res = object->ops->fault(object, region, fault_addr, fault_flags, &out_page, &out_flags);
    if (res == 0 && out_page) {
        if (active_hal_pt) {
            uint32_t pt_flags = region->prot; // Mock conversion
            active_hal_pt->map_page(aspace->root_pt, fault_addr & ~(PAGE_SIZE-1), out_page, pt_flags);
            hal_tlb_invalidate_page(aspace, fault_addr & ~(PAGE_SIZE-1));
        }
    }
    return res;
}
