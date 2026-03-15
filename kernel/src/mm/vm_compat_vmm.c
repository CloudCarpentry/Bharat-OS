#include "../../include/mm.h"
#include "../../include/hal/vmm.h"
#include "../../include/mm/vm_space.h"
#include "../../include/mm/vm_mapping.h"
#include "../../include/mm/arch_vm.h"
#include <stddef.h>

/*
 * Shim to bridge legacy `vmm.h` API to the new `vm_space_t` plane architecture.
 * This ensures the existing boot flow doesn't break while we transition.
 */

static vm_space_t *legacy_kernel_space = NULL;

static void init_legacy_shim(void) {
    if (!legacy_kernel_space) {
        vm_space_create(&legacy_kernel_space, MEM_PROFILE_MMU_BASIC, VM_TIMING_BEST_EFFORT);
        vm_activate_local(legacy_kernel_space);
    }
}

// Intercept map page to forward to the new API
int mm_vmm_map_page_shim(address_space_t* as, virt_addr_t vaddr, phys_addr_t paddr, uint32_t flags) {
    (void)as; // We redirect to the globally managed shim space for now if it matches kernel

    init_legacy_shim();

    vm_map_req_t req = {0};
    req.va = vaddr;
    req.pa = paddr;
    req.len = 4096; // Assume single page for legacy shim
    req.prot = VM_PROT_READ;
    if (flags & CAP_RIGHT_WRITE) req.prot |= VM_PROT_WRITE;
    if (flags & PAGE_USER) req.prot |= VM_PROT_USER;
    if (flags & PAGE_EXEC) req.prot |= VM_PROT_EXEC;

    req.mem_type = VM_MEM_NORMAL;
    req.map_flags = 0; // Best effort
    req.zone = VM_MEM_ZONE_NORMAL;
    req.timing_class = VM_TIMING_BEST_EFFORT;

    return vm_map(legacy_kernel_space, &req);
}

int mm_vmm_unmap_page_shim(address_space_t* as, virt_addr_t vaddr) {
    (void)as;
    init_legacy_shim();
    return vm_unmap(legacy_kernel_space, vaddr, 4096);
}
