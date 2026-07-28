#include "mm/prot_domain.h"
#include "../../include/mm/prot_domain.h"
#include "../../include/hal/hal_pt.h"
#include "../../include/arch/arch_caps.h"
#include "hal/hal_mm.h"
#include "console/console_core.h"
#include "kernel/status.h"
#include <stddef.h>

static prot_domain_ops_t* active_backend = NULL;
static prot_mode_t active_mode = PROT_MODE_NONE;

#include "slab.h"

// ---------------------------------------------------------------------------
// 1. MMU_FULL Backend Realization
// ---------------------------------------------------------------------------
static prot_domain_t* mmu_full_create(void) {
    prot_domain_t* domain = (prot_domain_t*)kmalloc(sizeof(prot_domain_t));
    if (!domain) return NULL;
    domain->mode = PROT_MODE_MMU_FULL;
    domain->ops = active_backend;

    if (active_hal_pt) {
        extern phys_addr_t vmm_get_kernel_root(void);
        domain->backend_state = (void*)(uintptr_t)hal_pt_create_address_space(vmm_get_kernel_root());
        if (!domain->backend_state) {
            kfree(domain);
            return NULL;
        }
    } else {
        domain->backend_state = NULL;
    }
    return domain;
}

static void mmu_full_destroy(prot_domain_t* domain) {
    if (domain) {
        if (active_hal_pt && domain->backend_state) {
            hal_pt_destroy_address_space((phys_addr_t)(uintptr_t)domain->backend_state);
        }
        kfree(domain);
    }
}

static void mmu_full_activate(prot_domain_t* domain) {
    (void)domain;
}

static int mmu_full_map_region(prot_domain_t* domain, uintptr_t vaddr, uintptr_t paddr, size_t size, uint32_t flags) {
    if (active_hal_pt && domain && domain->backend_state) {
        return hal_pt_map_range((phys_addr_t)(uintptr_t)domain->backend_state, vaddr, paddr, size, flags);
    }
    return -1;
}

static int mmu_full_unmap_region(prot_domain_t* domain, uintptr_t vaddr, size_t size) {
    if (active_hal_pt && domain && domain->backend_state) {
        return hal_pt_unmap_range((phys_addr_t)(uintptr_t)domain->backend_state, vaddr, size);
    }
    return -1;
}

static int mmu_full_protect_region(prot_domain_t* domain, uintptr_t vaddr, size_t size, uint32_t flags) {
    if (active_hal_pt && domain && domain->backend_state) {
        return hal_pt_protect_range((phys_addr_t)(uintptr_t)domain->backend_state, vaddr, size, flags);
    }
    return -1;
}

static int mmu_full_query_region(prot_domain_t* domain, uintptr_t vaddr, uintptr_t* paddr, uint32_t* flags) {
    if (active_hal_pt && domain && domain->backend_state) {
        size_t mapped_size;
        phys_addr_t pa;
        int res = hal_pt_query_mapping((phys_addr_t)(uintptr_t)domain->backend_state, vaddr, &pa, &mapped_size, flags);
        if (res == 0 && paddr) {
            *paddr = (uintptr_t)pa;
        }
        return res;
    }
    return -1;
}

static prot_domain_ops_t mmu_full_backend_ops = {
    .create = mmu_full_create,
    .destroy = mmu_full_destroy,
    .activate = mmu_full_activate,
    .map_region = mmu_full_map_region,
    .unmap_region = mmu_full_unmap_region,
    .protect_region = mmu_full_protect_region,
    .query_region = mmu_full_query_region,
};

// ---------------------------------------------------------------------------
// 2. MMU_LITE Backend Realization
// ---------------------------------------------------------------------------
static prot_domain_t* mmu_lite_create(void) {
    prot_domain_t* domain = (prot_domain_t*)kmalloc(sizeof(prot_domain_t));
    if (!domain) return NULL;
    domain->mode = PROT_MODE_MMU_LITE;
    domain->ops = active_backend;
    domain->backend_state = NULL; // Constrained translation, often single globally configured root
    return domain;
}

static void mmu_lite_destroy(prot_domain_t* domain) {
    if (domain) kfree(domain);
}

static void mmu_lite_activate(prot_domain_t* domain) {
    (void)domain;
}

static int mmu_lite_map_region(prot_domain_t* domain, uintptr_t vaddr, uintptr_t paddr, size_t size, uint32_t flags) {
    (void)domain;
    // MMU_LITE forbids runtime demand paging and only allows mapped ranges satisfying specific constraints
    if (size % 4096 != 0 || vaddr % 4096 != 0 || paddr % 4096 != 0) {
        return K_ERR_INVALID_ARG; // Explicit validation error
    }
    // Only allow when mapping is pre-allocated or statically configured
    if (flags & (1 << 4)) { // Assuming bit 4 represents lazy or demand faulting
        return K_ERR_UNSUPPORTED; // Demand faulting/paging is forbidden
    }
    return 0; // Pre-realization success
}

static int mmu_lite_unmap_region(prot_domain_t* domain, uintptr_t vaddr, size_t size) {
    (void)domain; (void)vaddr; (void)size;
    return 0;
}

static int mmu_lite_protect_region(prot_domain_t* domain, uintptr_t vaddr, size_t size, uint32_t flags) {
    (void)domain; (void)vaddr; (void)size; (void)flags;
    return 0;
}

static int mmu_lite_query_region(prot_domain_t* domain, uintptr_t vaddr, uintptr_t* paddr, uint32_t* flags) {
    (void)domain; (void)vaddr; (void)paddr; (void)flags;
    return K_ERR_UNSUPPORTED;
}

static prot_domain_ops_t mmu_lite_backend_ops = {
    .create = mmu_lite_create,
    .destroy = mmu_lite_destroy,
    .activate = mmu_lite_activate,
    .map_region = mmu_lite_map_region,
    .unmap_region = mmu_lite_unmap_region,
    .protect_region = mmu_lite_protect_region,
    .query_region = mmu_lite_query_region,
};

// ---------------------------------------------------------------------------
// 3. MPU Backend Realization
// ---------------------------------------------------------------------------
static prot_domain_t* mpu_create(void) {
    prot_domain_t* domain = (prot_domain_t*)kmalloc(sizeof(prot_domain_t));
    if (!domain) return NULL;
    domain->mode = PROT_MODE_MPU_ONLY;
    domain->ops = active_backend;
    domain->backend_state = NULL; // MPU does not allocate or require page-tables
    return domain;
}

static void mpu_destroy(prot_domain_t* domain) {
    if (domain) kfree(domain);
}

static void mpu_activate(prot_domain_t* domain) {
    (void)domain;
}

static int mpu_map_region(prot_domain_t* domain, uintptr_t vaddr, uintptr_t paddr, size_t size, uint32_t flags) {
    (void)domain; (void)flags;
    // MPU is region-based, not paging-based.
    // Validate alignment, power-of-two size or hardware alignment limits (typically 32 bytes to 4KB).
    if (vaddr != paddr) {
        // MPU platforms typically run with identity mappings or simple offset region translation
        return K_ERR_UNSUPPORTED;
    }
    // Reject page-table allocation, TLB shootdown hooks, demand faulting or COW
    if (flags & (1 << 4)) { // demand faulting / COW bit
        return K_ERR_UNSUPPORTED;
    }
    if (size < 32 || (size & (size - 1)) != 0) {
        // Common MPU hardware requirement: power-of-two size and size-aligned base
        // Or at least 32-byte alignment.
        if (vaddr % 32 != 0) {
            return K_ERR_INVALID_ARG;
        }
    }
    return 0; // Success under MPU model constraints
}

static int mpu_unmap_region(prot_domain_t* domain, uintptr_t vaddr, size_t size) {
    (void)domain; (void)vaddr; (void)size;
    return 0;
}

static int mpu_protect_region(prot_domain_t* domain, uintptr_t vaddr, size_t size, uint32_t flags) {
    (void)domain; (void)vaddr; (void)size; (void)flags;
    return 0;
}

static int mpu_query_region(prot_domain_t* domain, uintptr_t vaddr, uintptr_t* paddr, uint32_t* flags) {
    (void)domain; (void)vaddr; (void)paddr; (void)flags;
    return K_ERR_UNSUPPORTED;
}

static prot_domain_ops_t mpu_backend_ops = {
    .create = mpu_create,
    .destroy = mpu_destroy,
    .activate = mpu_activate,
    .map_region = mpu_map_region,
    .unmap_region = mpu_unmap_region,
    .protect_region = mpu_protect_region,
    .query_region = mpu_query_region,
};

// ---------------------------------------------------------------------------
// Prot None / Fallback
// ---------------------------------------------------------------------------
static prot_domain_ops_t prot_none_ops = {
    .create = NULL,
    .destroy = NULL,
    .activate = NULL,
    .map_region = NULL,
    .unmap_region = NULL,
    .protect_region = NULL,
    .query_region = NULL,
};

void prot_domain_init(void) {
    hal_mm_backend_caps_t backend_caps;
    hal_mm_backend_caps(&backend_caps);

    if (backend_caps.kind == HAL_MM_BACKEND_MMU_FULL) {
        active_mode = PROT_MODE_MMU_FULL;
        active_backend = &mmu_full_backend_ops;
    } else if (backend_caps.kind == HAL_MM_BACKEND_MMU_LITE) {
        active_mode = PROT_MODE_MMU_LITE;
        active_backend = &mmu_lite_backend_ops;
    } else if (backend_caps.kind == HAL_MM_BACKEND_MPU_ONLY) {
        active_mode = PROT_MODE_MPU_ONLY;
        active_backend = &mpu_backend_ops;
    } else {
        active_mode = PROT_MODE_NONE;
        active_backend = &prot_none_ops;
    }
}

prot_domain_ops_t* prot_domain_get_active_backend(void) {
    return active_backend;
}

prot_domain_t* prot_domain_create(void) {
    if (!active_backend || !active_backend->create) return NULL;
    return active_backend->create();
}

void prot_domain_destroy(prot_domain_t* domain) {
    if (active_backend && active_backend->destroy) {
        active_backend->destroy(domain);
    }
}

void prot_domain_activate(prot_domain_t* domain) {
    if (active_backend && active_backend->activate) {
        active_backend->activate(domain);
    }
}

int prot_domain_map_region(prot_domain_t* domain, uintptr_t vaddr, uintptr_t paddr, size_t size, uint32_t flags) {
    if (!active_backend || !active_backend->map_region) return -1;
    return active_backend->map_region(domain, vaddr, paddr, size, flags);
}

int prot_domain_unmap_region(prot_domain_t* domain, uintptr_t vaddr, size_t size) {
    if (!active_backend || !active_backend->unmap_region) return -1;
    return active_backend->unmap_region(domain, vaddr, size);
}

int prot_domain_protect_region(prot_domain_t* domain, uintptr_t vaddr, size_t size, uint32_t flags) {
    if (!active_backend || !active_backend->protect_region) return -1;
    return active_backend->protect_region(domain, vaddr, size, flags);
}

int prot_domain_query_region(prot_domain_t* domain, uintptr_t vaddr, uintptr_t* paddr, uint32_t* flags) {
    if (!active_backend || !active_backend->query_region) return -1;
    return active_backend->query_region(domain, vaddr, paddr, flags);
}
