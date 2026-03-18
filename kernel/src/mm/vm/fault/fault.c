#include "../../include/mm/fault.h"
#include "../../include/mm/aspace.h"
#include "../../include/mm/vm_object.h"
#include "../../include/mm.h"
#include "../../include/hal/hal_pt.h"

extern void hal_serial_write(const char *str);
#define KPRINT(s) hal_serial_write(s)

// Translates `vm_fault_event_t` access flags into the `vm_fault_ctx_t` format
// and dispatches the fault to the VM object's backend ops.
vm_fault_result_t vm_handle_fault(const vm_fault_event_t *event) {
    if (!event || !event->aspace || !active_hal_pt) {
        return VM_FAULT_PANIC;
    }

    uint64_t vaddr = event->fault_addr;

    // Find authoritative region backing this VA
    vm_region_t *region = NULL;
    int ret = aspace_find_region(event->aspace, vaddr, &region);

    if (ret != 0 || !region) {
        KPRINT("VM FAULT: No region found for VA\n");
        return VM_FAULT_KILL; // Unmapped memory access (SIGSEGV)
    }

    // Permission Check
    // If the fault was a write, does the region allow writes?
    if ((event->access & VM_FAULT_WRITE) && !(region->prot & HAL_PT_FLAG_WRITE)) {
        // Here we could handle Copy-on-Write (COW).
        // For standard faults, if write not allowed, kill.
        // In a full implementation, check if the region is marked COW.
        if (region->map_flags & HAL_PT_FLAG_COW) {
            // COW fault logic (to be handled)
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

    // Prepare Fault Context
    vm_fault_ctx_t ctx;
    ctx.fault_addr = vaddr;
    ctx.page_offset = (vaddr - region->base) + region->object_offset;
    ctx.access = 0;
    ctx.fault_flags = 0;

    if (event->access & VM_FAULT_READ)  ctx.access |= VM_FAULT_READ;
    if (event->access & VM_FAULT_WRITE) ctx.access |= VM_FAULT_WRITE;
    if (event->access & VM_FAULT_EXEC)  ctx.access |= VM_FAULT_EXEC;

    // Dispatch to Object Fault Handler
    phys_addr_t out_phys_page = 0;
    int obj_ret = region->object->ops->fault(region->object, &ctx, &out_phys_page);

    if (obj_ret == VM_FAULT_HANDLED && out_phys_page != 0) {
        // Map the returned physical page into the hardware page tables
        uint32_t pt_flags = region->prot | HAL_PT_FLAG_USER; // Defaulting to user access for user spaces

        // If this is a COW resolution, the object fault handler returns a new page
        // Ensure write flags are passed down correctly if it was a write fault that succeeded

        if (active_hal_pt->map_page(event->aspace->root_pt, vaddr, out_phys_page, pt_flags) != 0) {
             KPRINT("VM FAULT: Hardware map failed after resolution\n");
             return VM_FAULT_PANIC;
        }

        return VM_FAULT_RESOLVED;
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
