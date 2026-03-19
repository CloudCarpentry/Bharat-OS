#include "../../include/mm/vmm.h"
#include "../../include/mm/aspace.h"
#include "../../include/mm/vm_object.h"
#include "../../include/mm/pmm.h"
#include "../../include/hal/hal_pt.h"
#include "../../include/hal/hal_tlb.h"
#include "../../include/kernel.h"

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
    if ((fault_flags & CAP_RIGHT_WRITE) && !(region->flags & CAP_RIGHT_WRITE)) {
        return -2; // Permission fault
    }
    if ((fault_flags & CAP_RIGHT_EXECUTE) && !(region->flags & CAP_RIGHT_EXECUTE)) {
        return -2; // Execute permission fault
    }

    // 3. Delegate to the underlying object backend
    vm_object_t *object = region->object;
    if (!object || !object->ops || !object->ops->fault) {
        return -3; // No backing object or no fault handler
    }

    // Calculate offset within the object
    uint64_t offset_in_region = fault_addr - region->base;
    uint64_t object_offset = region->offset + offset_in_region;

    // Delegate to object backend (Anon/Shared/File/Device)
    return object->ops->fault(object, aspace, fault_addr, object_offset, fault_flags);
}
