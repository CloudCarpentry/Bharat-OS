#include "mm.h"
#include "mm/aspace.h"
#include "hal/hal_pt.h"
#include "hal/mmu_ops.h"
#include "hal/hal_tlb.h"
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
        // Safe copy to avoid SIMD traps during early boot
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
    // Phase A: Select capability-driven protection profile backend layer first
    // This must occur before any generic VMM logic attempts to create an address space/domain.
    prot_domain_init();

    ensure_kernel_space_ready();
    return kernel_space_ready ? 0 : -1;
}

phys_addr_t vmm_get_kernel_root(void) {
    return kernel_root_pt;
}

int mm_vmm_map_page(address_space_t* as, virt_addr_t vaddr, phys_addr_t paddr, uint32_t flags) {
    if (!as || !as->prot_domain) return -1;

    // Look up authoritative region
    vm_region_t *region = aspace_lookup_region(as, vaddr);
    (void)region;

    uint32_t mmu_flags = 0;
    if (flags & CAP_RIGHT_WRITE) mmu_flags |= MMU_WRITE;
    if (flags & CAP_RIGHT_EXECUTE) mmu_flags |= MMU_EXEC;
    if (flags & PAGE_USER) mmu_flags |= MMU_USER;
    if (flags & (CAP_RIGHT_DEVICE_GPU | CAP_RIGHT_DEVICE_NPU)) mmu_flags |= MMU_DEVICE;

    int ret = prot_domain_map_region(as->prot_domain, vaddr, paddr, PAGE_SIZE, mmu_flags);

    // In legacy MMU-only code, we updated active_hal_pt here
    // And handled a fallback for unmapped regions. The new backend natively handles it.
    if (ret == 0) {
        hal_tlb_invalidate_page(as, vaddr);
    }
    return ret;
}

int mm_vmm_unmap_page(address_space_t* as, virt_addr_t vaddr) {
    if (!as || !as->prot_domain) return -1;

    int ret = prot_domain_unmap_region(as->prot_domain, vaddr, PAGE_SIZE);
    if (ret == 0) {
        hal_tlb_invalidate_page(as, vaddr);
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

int vmm_handle_cow_fault(address_space_t* as, virt_addr_t vaddr) {
    extern int vm_handle_fault(address_space_t* as, virt_addr_t fault_addr, uint32_t fault_flags);
    return vm_handle_fault(as, vaddr, CAP_RIGHT_WRITE);
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

void tlb_shootdown(address_space_t *as, virt_addr_t vaddr) {
    hal_tlb_invalidate_page(as, vaddr);
}
