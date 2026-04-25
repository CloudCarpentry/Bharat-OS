#include "../kernel/include/hal/hal_pt.h"
#include "../kernel/include/hal/hal_tlb.h"
#include "../kernel/include/kernel.h"
#include "mm/tlb.h"
#include <stdbool.h>
#include <stddef.h>

hal_pt_ops_t *active_hal_pt = NULL;
hal_tlb_ops_t *active_hal_tlb = NULL;

static inline bool is_page_aligned_u64(uint64_t val) {
    return (val & (PAGE_SIZE - 1U)) == 0U;
}

void hal_pt_register_ops(hal_pt_ops_t *ops, hal_tlb_ops_t *tlb_ops) {
    if (ops) active_hal_pt = ops;
    if (tlb_ops) active_hal_tlb = tlb_ops;
}

// Architecture-specific initialization will call hal_pt_register_ops.
// This weak stub handles cases where no registration happens early.
void __attribute__((weak)) arch_hal_pt_init(void) {}

void hal_pt_init(void) {
    if (!active_hal_pt) {
        arch_hal_pt_init();
    }

#if defined(BHARAT_PROFILE_MPU_ONLY) || defined(PROFILE_MPU_ONLY)
    extern hal_pt_ops_t mpu_hal_pt_ops;
    extern hal_tlb_ops_t mpu_hal_tlb_ops;
    // Overrides arch-specific setups for bare-metal MPU configs
    active_hal_pt = &mpu_hal_pt_ops;
    active_hal_tlb = &mpu_hal_tlb_ops;
#endif
}

void hal_tlb_init(void) {
    if (!active_hal_tlb) {
        hal_pt_init();
    }
    tlb_init(); // Initialize the common core tlb subsystem.
}

phys_addr_t hal_pt_create_address_space(phys_addr_t kernel_root_table) {
    if (!active_hal_pt) {
        hal_pt_init();
    }
    if (!active_hal_pt || !active_hal_pt->create_address_space) {
        return 0U;
    }
    return active_hal_pt->create_address_space(kernel_root_table);
}

void hal_pt_destroy_address_space(phys_addr_t root_pt) {
    if (!active_hal_pt) {
        hal_pt_init();
    }
    if (!active_hal_pt || !active_hal_pt->destroy_address_space) {
        return;
    }
    active_hal_pt->destroy_address_space(root_pt);
}

int hal_pt_map_range(phys_addr_t root_pt, virt_addr_t vaddr, phys_addr_t paddr, size_t size, uint32_t flags) {
    if (!active_hal_pt) {
        hal_pt_init();
    }
    if (!active_hal_pt) {
        return -1;
    }

    if (size == 0U) {
        return 0;
    }

    if (!is_page_aligned_u64(vaddr) || !is_page_aligned_u64(paddr) || !is_page_aligned_u64(size)) {
        kernel_panic("hal_pt_map_range: unaligned parameters");
        return -1;
    }

    // Basic flag validation - ensure at least some permissions or special flags are set
    if (flags == 0) {
        return -1;
    }

    if (active_hal_pt->map_range) {
        return active_hal_pt->map_range(root_pt, vaddr, paddr, size, flags);
    }

    if (!active_hal_pt->map_page) {
        return -1;
    }

    for (size_t off = 0U; off < size; off += PAGE_SIZE) {
        int rc = active_hal_pt->map_page(root_pt, vaddr + off, paddr + off, flags);
        if (rc != 0) {
            (void)hal_pt_unmap_range(root_pt, vaddr, off);
            return rc;
        }
    }
    return 0;
}

int hal_pt_unmap_range(phys_addr_t root_pt, virt_addr_t vaddr, size_t size) {
    if (!active_hal_pt) {
        hal_pt_init();
    }
    if (!active_hal_pt) {
        return -1;
    }

    if (size == 0U) {
        return 0;
    }

    if (!is_page_aligned_u64(vaddr) || !is_page_aligned_u64(size)) {
        kernel_panic("hal_pt_unmap_range: unaligned parameters");
        return -1;
    }

    if (active_hal_pt->unmap_range) {
        return active_hal_pt->unmap_range(root_pt, vaddr, size);
    }

    if (!active_hal_pt->unmap_page) {
        return -1;
    }

    for (size_t off = 0U; off < size; off += PAGE_SIZE) {
        int rc = active_hal_pt->unmap_page(root_pt, vaddr + off, NULL);
        if (rc != 0) {
            return rc;
        }
    }
    return 0;
}

int hal_pt_protect_range(phys_addr_t root_pt, virt_addr_t vaddr, size_t size, uint32_t new_flags) {
    if (!active_hal_pt) {
        hal_pt_init();
    }
    if (!active_hal_pt || size == 0U || !is_page_aligned_u64(vaddr) || !is_page_aligned_u64(size)) {
        return -1;
    }

    if (active_hal_pt->protect_range) {
        return active_hal_pt->protect_range(root_pt, vaddr, size, new_flags);
    }

    if (!active_hal_pt->protect_page) {
        return -1;
    }

    for (size_t off = 0U; off < size; off += PAGE_SIZE) {
        int rc = active_hal_pt->protect_page(root_pt, vaddr + off, new_flags);
        if (rc != 0) {
            return rc;
        }
    }
    return 0;
}

int hal_pt_query_mapping(phys_addr_t root_pt, virt_addr_t vaddr, phys_addr_t *paddr, size_t *mapped_size, uint32_t *flags) {
    if (!active_hal_pt) {
        hal_pt_init();
    }
    if (!active_hal_pt) {
        return -1;
    }

    if (active_hal_pt->query_mapping) {
        return active_hal_pt->query_mapping(root_pt, vaddr, paddr, mapped_size, flags);
    }

    if (!active_hal_pt->query_page) {
        return -1;
    }

    int rc = active_hal_pt->query_page(root_pt, vaddr, paddr, flags);
    if (rc != 0) {
        return rc;
    }
    if (mapped_size) {
        *mapped_size = PAGE_SIZE;
    }
    return 0;
}

const hal_pt_caps_t *hal_pt_caps(void) {
    if (!active_hal_pt) {
        hal_pt_init();
    }
    if (!active_hal_pt || !active_hal_pt->caps) {
        return NULL;
    }
    return active_hal_pt->caps;
}

const hal_tlb_caps_t *hal_tlb_caps(void) {
    if (!active_hal_tlb) {
        hal_pt_init();
    }
    if (!active_hal_tlb || !active_hal_tlb->caps) {
        return NULL;
    }
    return active_hal_tlb->caps;
}
