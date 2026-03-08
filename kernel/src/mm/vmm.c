#include "../../include/mm.h"
#include "../../include/numa.h"
#include "../../include/hal/hal.h"
#include "../../include/hal/vmm.h"
#include "../../include/advanced/formal_verif.h"
#include "../../include/sched.h"
#include "../../include/capability.h"

// @cite L4 Microkernels: The Lessons from 20 Years of Research and Implementation (Klein & Andronick, 2016)
// L4 minimal memory mapping model: Kernel only maps/unmaps physical pages, policy lives in user space.
#include <stddef.h>
#include <stdint.h>

#define P2V(x) ((void*)(uintptr_t)(x))
#define V2P(x) ((phys_addr_t)(uintptr_t)(x))

static address_space_t kernel_space;

int vmm_init(void) {
    phys_addr_t root_dir_phys = hal_vmm_init_root();
    if (root_dir_phys == 0U) {
        return -1;
    }

    kernel_space.root_table = root_dir_phys;
    return 0;
}

int mm_vmm_map_page(address_space_t* as, virt_addr_t vaddr, phys_addr_t paddr, uint32_t flags) {
    if (!as || as->root_table == 0U || paddr == 0U) {
        return -1;
    }

    if ((flags & PAGE_COW) != 0U) {
        mm_inc_page_ref(paddr);
        flags &= ~CAP_RIGHT_WRITE;
    }

    return hal_vmm_map_page(as->root_table, vaddr, paddr, flags);
}

// Global TLB shootdown function
void tlb_shootdown(virt_addr_t vaddr) {
    hal_tlb_flush((unsigned long long)vaddr);

    // IPI based shootdown for cores executing the same process (or system wide for kernel mappings)
    uint32_t current_core = hal_cpu_get_id();
    for (uint32_t i = 0; i < 8; ++i) { // MAX_SUPPORTED_CORES is 8
        if (i != current_core) {
            hal_send_ipi_payload(i, (uint64_t)vaddr);
        }
    }
}

int mm_vmm_unmap_page(address_space_t* as, virt_addr_t vaddr) {
    if (!as || as->root_table == 0U) {
        return -1;
    }

    phys_addr_t paddr = 0;
    int ret = hal_vmm_unmap_page(as->root_table, vaddr, &paddr);
    if (ret < 0) {
        return ret;
    }

    if (paddr != 0) {
        mm_free_page(paddr);
    }

    tlb_shootdown(vaddr);
    return 0;
}

int vmm_map_page(virt_addr_t vaddr, phys_addr_t paddr, uint32_t flags) {
    return mm_vmm_map_page(&kernel_space, vaddr, paddr, flags);
}

int vmm_unmap_page(virt_addr_t vaddr) {
    return mm_vmm_unmap_page(&kernel_space, vaddr);
}

int vmm_map_device_mmio(virt_addr_t vaddr, phys_addr_t paddr, capability_t* cap, int is_npu) {
    if (!cap) {
        return -3; // ERR_CAP_DENIED / IPC_ERR_WOULD_BLOCK
    }

    uint32_t required_right = is_npu ? CAP_RIGHT_DEVICE_NPU : CAP_RIGHT_DEVICE_GPU;
    if ((cap->rights_mask & required_right) == 0U) {
        return -3; // ERR_CAP_DENIED
    }

    return mm_vmm_map_page(&kernel_space, vaddr, paddr, CAP_RIGHT_READ | CAP_RIGHT_WRITE);
}

address_space_t* mm_create_address_space(void) {
    phys_addr_t root = hal_vmm_setup_address_space(kernel_space.root_table);
    if (root == 0U) {
        return NULL;
    }

    address_space_t* as = (address_space_t*)(uintptr_t)mm_alloc_page(NUMA_NODE_ANY);
    if (!as) {
        mm_free_page(root);
        return NULL;
    }

    as->root_table = root;
    return as;
}

// Copy-on-Write Fault Handler
int vmm_handle_cow_fault(address_space_t* as, virt_addr_t vaddr) {
    if (!as || as->root_table == 0U) return -1;

    phys_addr_t old_phys = 0;
    uint32_t old_flags = 0;

    int ret = hal_vmm_get_mapping(as->root_table, vaddr, &old_phys, &old_flags);
    if (ret < 0 || old_phys == 0) return -2;

    // Allocate new page
    phys_addr_t new_phys = mm_alloc_page(NUMA_NODE_ANY);
    if (!new_phys) return -1;

    // Copy data
    uint8_t* src = (uint8_t*)P2V(old_phys);
    uint8_t* dst = (uint8_t*)P2V(new_phys);
    for (int i = 0; i < PAGE_SIZE; i++) {
        dst[i] = src[i];
    }

    // Update mapping with Write permission, clear CoW
    uint32_t new_flags = (old_flags | CAP_RIGHT_WRITE) & ~PAGE_COW;

    ret = hal_vmm_update_mapping(as->root_table, vaddr, new_phys, new_flags);
    if (ret < 0) {
        mm_free_page(new_phys);
        return ret;
    }

    // Decrement old page reference
    mm_free_page(old_phys);

    tlb_shootdown(vaddr);
    return 0;
}
