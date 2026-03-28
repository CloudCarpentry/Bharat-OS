#include "mm.h"
#include "mm/aspace.h"
#include "hal/hal_mpa.h"
#include "hal/hal_tlb.h"
#include "hal/hal_pt.h"
#include "mm/tlb.h"
#include "capability.h"
#include "mm/pmm.h"
#include "slab.h"
#include <stddef.h>
#include <stdint.h>

address_space_t kernel_space;
static int kernel_space_ready = 0;
static phys_addr_t kernel_root_pt = 0;
static volatile int kernel_space_init_in_progress = 0;

static void ensure_kernel_space_ready(void) {
    if (kernel_space_ready) return;
    if (kernel_space_init_in_progress) return;

    kernel_space_init_in_progress = 1;

    address_space_t *created = NULL;
    if (aspace_create(&created, 0) == 0 && created) {
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

#include "mm/prot_domain.h"

int vmm_init(void) {
    prot_domain_init();
    ensure_kernel_space_ready();

    if (kernel_space_ready && kernel_space.root_pt != 0) {
        if (active_mem_protect && active_mem_protect->cpu_ops.set_root) {
            active_mem_protect->cpu_ops.set_root(kernel_space.root_pt);
        }
    }

    return kernel_space_ready ? 0 : -1;
}

phys_addr_t vmm_get_kernel_root(void) {
    if (kernel_root_pt != 0) return kernel_root_pt;
    if (active_mem_protect && active_mem_protect->cpu_ops.get_root) {
        return active_mem_protect->cpu_ops.get_root();
    }
    return 0;
}

int mm_vmm_map_page(address_space_t* as, virt_addr_t vaddr, phys_addr_t paddr, uint32_t flags) {
    if (!as) return -1;

    // Use the unified MPA HAL abstraction instead of the old MMU ops or prot_domain
    if (!active_mem_protect || !active_mem_protect->cpu_ops.map_page) return -1;

    // Look up authoritative region (kept for legacy/bookkeeping compatibility)
    vm_region_t *region = aspace_lookup_region(as, vaddr);
    (void)region;

    // Translate VMM flags to MPA Capability Bits
    uint32_t mpa_flags = 0;
    if (flags & HAL_PT_FLAG_WRITE) mpa_flags |= MPA_CAP_WRITE;
    if (flags & HAL_PT_FLAG_EXEC) mpa_flags |= MPA_CAP_EXEC_PERM;
    if (flags & PAGE_USER) mpa_flags |= MPA_CAP_USER;
    if (flags & (0x40 | 0x80)) mpa_flags |= MPA_CAP_DEVICE; // Old GPU and NPU masks

    // Use the HAL abstraction
    int ret = active_mem_protect->cpu_ops.map_page(as->root_pt, vaddr, paddr, mpa_flags);

    if (ret == 0 && active_mem_protect->cpu_ops.flush_tlb_local) {
        active_mem_protect->cpu_ops.flush_tlb_local(vaddr, 0); // ASID 0 for now
    }
    return ret;
}

int mm_vmm_unmap_page(address_space_t* as, virt_addr_t vaddr) {
    if (!as) return -1;

    if (!active_mem_protect || !active_mem_protect->cpu_ops.unmap_page) return -1;

    phys_addr_t unmapped_pa;
    int ret = active_mem_protect->cpu_ops.unmap_page(as->root_pt, vaddr, &unmapped_pa);

    if (ret == 0 && active_mem_protect->cpu_ops.flush_tlb_local) {
        active_mem_protect->cpu_ops.flush_tlb_local(vaddr, 0); // ASID 0 for now
    }
    return ret;
}

int vmm_map_page(virt_addr_t vaddr, phys_addr_t paddr, uint32_t flags) {
    ensure_kernel_space_ready();
    return mm_vmm_map_page(&kernel_space, vaddr, paddr, flags);
}

int vmm_unmap_page(virt_addr_t vaddr) {
    ensure_kernel_space_ready();
    return mm_vmm_unmap_page(&kernel_space, vaddr);
}

#include "mm/fault.h"
#include "mm/vm_mapping.h"

int vmm_handle_cow_fault(address_space_t* as, virt_addr_t vaddr) {
    vm_fault_event_t event = {
        .aspace = as,
        .fault_addr = vaddr,
        .access = VM_PROT_WRITE,
        .arch_code = 0
    };
    vm_fault_result_t res = vm_handle_fault(&event);
    return (res == VM_FAULT_RESOLVED) ? 0 : -1;
}

address_space_t *mm_create_address_space(void) {
    address_space_t *as = NULL;
    if (aspace_create(&as, 0) != 0) return NULL;
    return as;
}

void vmm_process_local_urpc_messages(uint32_t core_id) {
    (void)core_id;
    extern void vmm_process_urpc_messages(void);
    vmm_process_urpc_messages();
}

int vmm_map_device_mmio(virt_addr_t vaddr, phys_addr_t paddr, capability_t *cap, int is_npu) {
    (void)is_npu;
    if (!cap) return -1;
    return vmm_map_page(vaddr, paddr, cap->rights_mask);
}
