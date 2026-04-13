#include "../../../include/mm.h"
#include "../../../include/mm/aspace.h"
#include "../../../include/hal/hal_pt.h"
#include "../../../include/hal/hal_tlb.h"
#include "../../../include/mm/tlb.h"
#include "../../../include/mm/pmm.h"
#include "../../../include/slab.h"
#include "../../../include/mm/aspace_profile.h"

// M1: Legacy VMM is now just a thin shim over ASpace/Region/Object layers.

void vmm_process_urpc_messages(void);

int mm_vmm_map_page(address_space_t* as, virt_addr_t vaddr, phys_addr_t paddr, uint32_t flags) {
    if (!as || !active_hal_pt) return -1;

    // Look up authoritative region
    vm_region_t *region = aspace_lookup_region(as, vaddr);
    if (!region) {
        // Fallback for kernel direct mappings or legacy code without regions
        uint32_t pt_flags = HAL_PT_FLAG_READ;
        if (flags & CAP_RIGHT_WRITE) pt_flags |= HAL_PT_FLAG_WRITE;
        if (flags & PAGE_USER) pt_flags |= HAL_PT_FLAG_USER;
        if (flags & (0x40 | 0x80)) pt_flags |= HAL_PT_FLAG_DEVICE; // Old GPU and NPU masks
        return active_hal_pt->map_page(as->root_pt, vaddr, paddr, pt_flags);
    }

    // Normally we'd map via region->object, but this is a legacy compat shim.
    uint32_t pt_flags = HAL_PT_FLAG_READ;
    if (flags & CAP_RIGHT_WRITE) pt_flags |= HAL_PT_FLAG_WRITE;
    if (flags & CAP_RIGHT_EXECUTE) pt_flags |= HAL_PT_FLAG_EXEC;
    if (flags & PAGE_USER) pt_flags |= HAL_PT_FLAG_USER;
    if (flags & (0x40 | 0x80)) pt_flags |= HAL_PT_FLAG_DEVICE; // Old GPU and NPU masks

    int ret = active_hal_pt->map_page(as->root_pt, vaddr, paddr, pt_flags);
    if (ret == 0) {
        tlb_invalidate_all(as, vaddr, PAGE_SIZE, TLB_INV_PAGE);
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
        tlb_invalidate_all(as, vaddr, PAGE_SIZE, TLB_INV_PAGE);
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

#include "../../../include/mm/fault.h"

int vmm_handle_cow_fault(address_space_t* as, virt_addr_t vaddr) {
    // Legacy shim simply routes back to the authoritative fault handler
    vm_fault_event_t event = {
        .aspace = as,
        .fault_addr = vaddr,
        .access = CAP_RIGHT_WRITE,
        .arch_code = 0
    };
    vm_fault_result_t res = vm_handle_fault(&event);
    return (res == VM_FAULT_RESOLVED) ? 0 : -1;
}

// TLB shootdown uses the core tlb abstraction now.

address_space_t kernel_space;
static int kernel_space_ready = 0;
static phys_addr_t kernel_root_pt = 0;

static volatile int kernel_space_init_in_progress = 0;

static void ensure_kernel_space_ready(void) {
    if (kernel_space_ready) return;
    if (kernel_space_init_in_progress) {
        return; // Avoid recursion if aspace_create calls back
    }

    kernel_space_init_in_progress = 1;

    address_space_t *created = NULL;
    if (aspace_create(&created, 0) == 0 && created) {
        // Use a volatile ptr loop to copy the structure byte-by-byte.
        // This avoids compiler-generated SSE/SIMD instructions (like movups)
        // for large struct assignments which trap on x86_64 during early boot
        // before the FPU/SSE state is fully initialized.
        volatile uint8_t *dest = (volatile uint8_t *)&kernel_space;
        volatile uint8_t *src  = (volatile uint8_t *)created;
        for (size_t i = 0; i < sizeof(address_space_t); i++) {
            dest[i] = src[i];
        }
        kfree(created);
        kernel_root_pt = kernel_space.root_pt;
        kernel_space_ready = 1;
    }

    kernel_space_init_in_progress = 0;
}

int vmm_init(void) {
    extern void prot_domain_init(void);
    prot_domain_init();
    ensure_kernel_space_ready();
    return kernel_space_ready ? 0 : -1;
}

phys_addr_t vmm_get_kernel_root(void) {
    // Return the current kernel root directly to avoid infinite recursion
    // when aspace_create is called during bootstrap (which will get 0).
    return kernel_root_pt;
}

address_space_t *mm_create_address_space(void) {
    aspace_profile_t profile = aspace_profile_get_current();
    // Do not assume full VM creation if the profile does not support it
    if (profile == ASPACE_PROFILE_REGION_ONLY) {
        // Reject full VM address space creation entirely or pass explicit region-only flags in the future
    }

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
