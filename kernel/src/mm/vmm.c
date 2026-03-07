#include "../../include/mm.h"
#include "../../include/numa.h"
#include "../../include/hal/hal.h"
#include "../../include/advanced/formal_verif.h"

#include <stddef.h>
#include <stdint.h>

/*
 * Bharat-OS Virtual Memory Manager
 * Architecture-neutral software mapping table for early bring-up.
 * HAL-backed architecture paging can consume these mappings per target.
 */

#define VMM_MAX_MAPPINGS 2048U
#define VMM_FLAG_PRESENT 0x1U

typedef struct {
    virt_addr_t vaddr;
    phys_addr_t paddr;
    uint32_t flags;
    uint8_t in_use;
} vmm_mapping_t;

static address_space_t kernel_space;
static vmm_mapping_t kernel_mappings[VMM_MAX_MAPPINGS];

static virt_addr_t align_down(virt_addr_t value) {
    return value & ~(virt_addr_t)(PAGE_SIZE - 1U);
}

static int vmm_find_slot(virt_addr_t vaddr) {
    for (uint32_t i = 0; i < VMM_MAX_MAPPINGS; ++i) {
        if (kernel_mappings[i].in_use != 0U && kernel_mappings[i].vaddr == vaddr) {
            return (int)i;
        }
    }
    return -1;
}

static int vmm_find_free_slot(void) {
    for (uint32_t i = 0; i < VMM_MAX_MAPPINGS; ++i) {
        if (kernel_mappings[i].in_use == 0U) {
            return (int)i;
        }
    }
    return -1;
}

int vmm_init(void) {
    phys_addr_t root_dir_phys = mm_alloc_page(NUMA_NODE_ANY);
    if (root_dir_phys == 0U) {
        return -1;
    }

    kernel_space.root_table = root_dir_phys;

    for (uint32_t i = 0; i < VMM_MAX_MAPPINGS; ++i) {
        kernel_mappings[i].in_use = 0U;
        kernel_mappings[i].vaddr = 0U;
        kernel_mappings[i].paddr = 0U;
        kernel_mappings[i].flags = 0U;
    }

    return 0;
}

int mm_vmm_map_page(address_space_t* as, virt_addr_t vaddr, phys_addr_t paddr, uint32_t flags) {
    if (!as || as->root_table == 0U || paddr == 0U) {
        return -1;
    }

    virt_addr_t aligned_vaddr = align_down(vaddr);
    phys_addr_t aligned_paddr = (phys_addr_t)align_down((virt_addr_t)paddr);

    int existing = vmm_find_slot(aligned_vaddr);
    int slot = existing;
    if (slot < 0) {
        slot = vmm_find_free_slot();
    }
    if (slot < 0) {
        return -2;
    }

    if ((flags & PAGE_COW) != 0U) {
        mm_inc_page_ref(aligned_paddr);
        flags &= ~CAP_RIGHT_WRITE;
    }

    kernel_mappings[slot].vaddr = aligned_vaddr;
    kernel_mappings[slot].paddr = aligned_paddr;
    kernel_mappings[slot].flags = flags | VMM_FLAG_PRESENT;
    kernel_mappings[slot].in_use = 1U;

    return 0;
}

int mm_vmm_unmap_page(address_space_t* as, virt_addr_t vaddr) {
    if (!as || as->root_table == 0U) {
        return -1;
    }

    int slot = vmm_find_slot(align_down(vaddr));
    if (slot < 0) {
        return -2;
    }

    kernel_mappings[slot].in_use = 0U;
    kernel_mappings[slot].vaddr = 0U;
    kernel_mappings[slot].paddr = 0U;
    kernel_mappings[slot].flags = 0U;

    hal_tlb_flush(vaddr);
    return 0;
}

/* Compatibility wrappers used by existing call-sites/docs. */
int vmm_map_page(virt_addr_t vaddr, phys_addr_t paddr, uint32_t flags) {
    return mm_vmm_map_page(&kernel_space, vaddr, paddr, flags);
}

int vmm_unmap_page(virt_addr_t vaddr) {
    return mm_vmm_unmap_page(&kernel_space, vaddr);
}

int vmm_map_device_mmio(virt_addr_t vaddr, phys_addr_t paddr, capability_t* cap, int is_npu) {
    if (!cap) {
        return -1;
    }

    uint32_t required_right = is_npu ? CAP_RIGHT_DEVICE_NPU : CAP_RIGHT_DEVICE_GPU;
    if ((cap->rights_mask & required_right) == 0U) {
        return -2;
    }

    return mm_vmm_map_page(&kernel_space, vaddr, paddr, CAP_RIGHT_READ | CAP_RIGHT_WRITE);
}

address_space_t* mm_create_address_space(void) {
    phys_addr_t root = mm_alloc_page(NUMA_NODE_ANY);
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
