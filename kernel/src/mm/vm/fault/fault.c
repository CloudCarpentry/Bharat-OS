#include "../../../../include/mm.h"
#include "../../../../include/mm/aspace.h"
#include "../../../../include/mm/vm_object.h"
#include "../../../../include/mm/pmm.h"
#include "../../../../include/hal/hal_pt.h"
#include "../../../../include/hal/hal_tlb.h"

// VM Fault Engine: Core page fault dispatcher and handler

int vm_handle_fault(address_space_t *aspace, virt_addr_t fault_addr, uint32_t fault_flags) {
    if (!aspace || !active_hal_pt) {
        return VM_FAULT_SIGSEGV;
    }

    vm_region_t *region = aspace_lookup_region(aspace, fault_addr);
    if (!region) {
        return VM_FAULT_SIGSEGV;
    }

    if ((fault_flags & CAP_RIGHT_WRITE) && !(region->prot & CAP_RIGHT_WRITE)) {
        return VM_FAULT_SIGSEGV;
    }
    if ((fault_flags & CAP_RIGHT_EXECUTE) && !(region->prot & CAP_RIGHT_EXECUTE)) {
        return VM_FAULT_SIGSEGV;
    }

    vm_object_t *object = region->object;
    if (!object || !object->ops || !object->ops->fault) {
        return VM_FAULT_SIGBUS;
    }

    phys_addr_t paddr = 0;
    uint32_t page_flags = 0;
    int ret = object->ops->fault(object,
                                 region,
                                 fault_addr,
                                 fault_flags,
                                 &paddr,
                                 &page_flags);
    if (ret != VM_FAULT_HANDLED) {
        return ret;
    }

    virt_addr_t aligned_vaddr = fault_addr & ~(PAGE_SIZE - 1ULL);
    ret = active_hal_pt->map_page(aspace->root_pt, aligned_vaddr, paddr, page_flags);
    if (ret != 0) {
        mm_free_page(paddr);
        return VM_FAULT_SIGBUS;
    }

    hal_tlb_invalidate_page(aspace, aligned_vaddr);
    return VM_FAULT_HANDLED;
}
