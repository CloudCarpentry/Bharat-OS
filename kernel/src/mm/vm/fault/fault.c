#include "../../include/mm/fault.h"
#include "../../include/mm/aspace.h"
#include "../../include/mm/vm_object.h"
#include "../../include/mm.h"
#include "../../include/hal/hal_pt.h"
#include "../../include/hal/hal.h"
#include "bharat/console.h"

#define KPRINT(s) console_write_raw(s)

// Translates `vm_fault_event_t` access flags into the `vm_fault_ctx_t` format
// and dispatches the fault to the VM object's backend ops.
vm_fault_result_t vm_handle_fault(const vm_fault_event_t *event) {
    if (!event || !event->aspace || !active_hal_pt) {
        return VM_FAULT_PANIC;
    }

    uintptr_t vaddr = event->fault_addr;

    // Check Real-Time Policy: Hard RT tasks should not experience demand paging faults
    // outside of controlled initialization phases. A page fault here is often a timing violation.
    if (event->aspace->timing_class >= 3) { // VM_TIMING_HARD_RT
         KPRINT("VM FAULT: Policy violation (Demand paging in HARD_RT space)\n");
         // For HARD_RT, a dynamic fault is often fatal unless explicitly handled
         return VM_FAULT_KILL;
    }

    // Find authoritative region backing this VA
    vm_region_t *region = aspace_lookup_region(event->aspace, vaddr);

    // Stack Growth Check (must happen before exact region overlap check fails)
    // If we missed the region, check if it's a stack region immediately above us.
    if (!region) {
        // Find if there is a region immediately above this fault address
        vm_region_t *next_region = event->aspace->regions;
        while (next_region && next_region->base <= vaddr) {
            next_region = next_region->next;
        }

        if (next_region && (next_region->region_flags & VM_REGION_FLAG_STACK)) {
            size_t dist = next_region->base - vaddr;
            if (dist <= 1024 * 1024) { // 1MB bounds check
                uintptr_t new_base = vaddr & ~(PAGE_SIZE - 1);

                // Expand region downwards
                size_t growth = next_region->base - new_base;
                next_region->length += growth;
                next_region->base = new_base;

                // Adjust object offset so existing mapped data remains at the same offset
                // relative to the beginning of the object.
                next_region->object_offset += growth;

                if (next_region->object) {
                    // Update object size if applicable, or keep as anonymous mapping
                    next_region->object->size += growth;
                }
                region = next_region; // We resolved the region via growth
            }
        }
    }

    if (!region) {
        KPRINT("VM FAULT: No region found for VA\n");
        return VM_FAULT_KILL; // Unmapped memory access (SIGSEGV)
    }

    // Permission Check
    // If the fault was a write, does the region allow writes?
    if ((event->access & VM_FAULT_WRITE) && !(region->prot & HAL_PT_FLAG_WRITE)) {
        // Here we intercept COW and delegate to the object fault handler.
        // We do NOT modify region->prot globally, nor do we query active_hal_pt.
        if (region->region_flags & VM_REGION_FLAG_COW) {
            // Re-route to object handler with explicit COW request flag.
            // We'll let the standard Object Fault Dispatch handle this below,
            // by passing the VM_FAULT_WRITE flag. The object handler will detect
            // the region is COW and do the right per-page resolution.
        } else {
             KPRINT("VM FAULT: Permission denied (Write on RO region)\n");
             return VM_FAULT_KILL; // Permission violation
        }
    }

    // Execute check
    if ((event->access & VM_FAULT_EXEC) && !(region->prot & HAL_PT_FLAG_EXEC)) {
         KPRINT("VM FAULT: Permission denied (Exec on NX region)\n");
         return VM_FAULT_KILL;
    }

    if (!region->object || !region->object->ops || !region->object->ops->fault) {
        KPRINT("VM FAULT: Region has no backing object or fault handler\n");
        return VM_FAULT_KILL;
    }

    uint32_t access = 0;
    if (event->access & VM_FAULT_READ)  access |= VM_FAULT_READ;
    if (event->access & VM_FAULT_WRITE) access |= VM_FAULT_WRITE;
    if (event->access & VM_FAULT_EXEC)  access |= VM_FAULT_EXEC;

    // Dispatch to Object Fault Handler
    phys_addr_t out_phys_page = 0;
    uint32_t out_page_flags = 0;

    int obj_ret = region->object->ops->fault(region->object, region, vaddr, access, &out_phys_page, &out_page_flags);

    if (obj_ret == VM_FAULT_HANDLED && out_phys_page != 0) {
        // Map the returned physical page into the hardware page tables
        uint32_t pt_flags = region->prot | HAL_PT_FLAG_USER | out_page_flags; // Defaulting to user access for user spaces

        // If this is a COW resolution, the object fault handler returns a new page
        // Ensure write flags are passed down correctly if it was a write fault that succeeded
        if ((access & VM_FAULT_WRITE) && (region->region_flags & VM_REGION_FLAG_COW)) {
            pt_flags |= HAL_PT_FLAG_WRITE;
        }

        if (active_hal_pt->map_page(event->aspace->root_pt, vaddr, out_phys_page, pt_flags) != 0) {
             KPRINT("VM FAULT: Hardware map failed after resolution\n");
             return VM_FAULT_PANIC;
        }

        // Issue a TLB flush for the page just updated (useful for COW replacements)
        hal_tlb_flush(vaddr);

        return VM_FAULT_RESOLVED;
    } else if (obj_ret == VM_FAULT_ENOSYS) {
        KPRINT("VM FAULT: Object fault handler is a stub (ENOSYS)\n");
        return VM_FAULT_KILL; // Not implemented yet
    } else if (obj_ret == VM_FAULT_OOM) {
        KPRINT("VM FAULT: OOM during fault resolution\n");
        return VM_FAULT_KILL; // Out of Memory
    } else if (obj_ret == VM_FAULT_SIGBUS) {
        KPRINT("VM FAULT: Bus error (SIGBUS) during fault resolution\n");
        return VM_FAULT_KILL;
    }

    KPRINT("VM FAULT: Object failed to handle fault\n");
    return VM_FAULT_KILL;
}
