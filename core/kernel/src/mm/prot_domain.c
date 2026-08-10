#include "mm/prot_domain.h"
#include "../../include/mm/prot_domain.h"
#include "../../include/hal/hal_pt.h"
#include "../../include/hal/hal_mpu.h"
#include "../../include/arch/arch_caps.h"
#include "hal/hal_mm.h"
#include "hal/hal_mpa.h"
#include "console/console_core.h"
#include "kernel/status.h"
#include <stddef.h>

static prot_domain_ops_t* active_backend = NULL;
static prot_mode_t active_mode = PROT_MODE_NONE;

#include "slab.h"

hal_mpu_ops_t *active_hal_mpu = NULL;
void hal_mpu_register_ops(hal_mpu_ops_t *ops) {
    active_hal_mpu = ops;
}

// Forward declarations of ops
static prot_domain_ops_t mmu_full_backend_ops;
static prot_domain_ops_t mmu_lite_backend_ops;
static prot_domain_ops_t mpu_backend_ops;
static prot_domain_ops_t prot_none_ops;

// ---------------------------------------------------------------------------
// 1. MMU_FULL Backend Realization
// ---------------------------------------------------------------------------
static int mmu_full_create(prot_domain_t** out_domain) {
    prot_domain_t* domain = (prot_domain_t*)kmalloc(sizeof(prot_domain_t));
    if (!domain) return K_ERR_NO_MEMORY;
    domain->mode = PROT_MODE_MMU_FULL;
    domain->ops = &mmu_full_backend_ops;

    if (active_hal_pt) {
        extern phys_addr_t vmm_get_kernel_root(void);
        domain->backend_state = (void*)(uintptr_t)hal_pt_create_address_space(vmm_get_kernel_root());
        if (!domain->backend_state) {
            kfree(domain);
            return K_ERR_NO_MEMORY;
        }
    } else {
        kfree(domain);
        return K_ERR_UNSUPPORTED;
    }
    *out_domain = domain;
    return K_OK;
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
    if (!domain || !domain->backend_state) {
        return;
    }
    if (active_mem_protect && active_mem_protect->cpu_ops.set_root) {
        active_mem_protect->cpu_ops.set_root((phys_addr_t)(uintptr_t)domain->backend_state);
    }
}

static int mmu_full_map_region(prot_domain_t* domain, uintptr_t vaddr, uintptr_t paddr, size_t size, uint32_t flags) {
    if (active_hal_pt && domain && domain->backend_state) {
        return hal_pt_map_range((phys_addr_t)(uintptr_t)domain->backend_state, vaddr, paddr, size, flags);
    }
    return K_ERR_UNSUPPORTED;
}

static int mmu_full_unmap_region(prot_domain_t* domain, uintptr_t vaddr, size_t size) {
    if (active_hal_pt && domain && domain->backend_state) {
        return hal_pt_unmap_range((phys_addr_t)(uintptr_t)domain->backend_state, vaddr, size);
    }
    return K_ERR_UNSUPPORTED;
}

static int mmu_full_protect_region(prot_domain_t* domain, uintptr_t vaddr, size_t size, uint32_t flags) {
    if (active_hal_pt && domain && domain->backend_state) {
        return hal_pt_protect_range((phys_addr_t)(uintptr_t)domain->backend_state, vaddr, size, flags);
    }
    return K_ERR_UNSUPPORTED;
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
    return K_ERR_UNSUPPORTED;
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
static int mmu_lite_create(prot_domain_t** out_domain) {
    prot_domain_t* domain = (prot_domain_t*)kmalloc(sizeof(prot_domain_t));
    if (!domain) return K_ERR_NO_MEMORY;
    domain->mode = PROT_MODE_MMU_LITE;
    domain->ops = &mmu_lite_backend_ops;
    domain->backend_state = NULL;

    if (active_hal_pt) {
        extern phys_addr_t vmm_get_kernel_root(void);
        domain->backend_state = (void*)(uintptr_t)hal_pt_create_address_space(vmm_get_kernel_root());
    } else {
        kfree(domain);
        return K_ERR_UNSUPPORTED;
    }
    *out_domain = domain;
    return K_OK;
}

static void mmu_lite_destroy(prot_domain_t* domain) {
    if (domain) {
        if (active_hal_pt && domain->backend_state) {
            hal_pt_destroy_address_space((phys_addr_t)(uintptr_t)domain->backend_state);
        }
        kfree(domain);
    }
}

static void mmu_lite_activate(prot_domain_t* domain) {
    if (!domain || !domain->backend_state) {
        return;
    }
    if (active_mem_protect && active_mem_protect->cpu_ops.set_root) {
        active_mem_protect->cpu_ops.set_root((phys_addr_t)(uintptr_t)domain->backend_state);
    }
}

static int mmu_lite_map_region(prot_domain_t* domain, uintptr_t vaddr, uintptr_t paddr, size_t size, uint32_t flags) {
    if (size % 4096 != 0 || vaddr % 4096 != 0 || paddr % 4096 != 0) {
        return K_ERR_INVALID_ARG;
    }
    if (active_hal_pt && domain && domain->backend_state) {
        return hal_pt_map_range((phys_addr_t)(uintptr_t)domain->backend_state, vaddr, paddr, size, flags);
    }
    return K_ERR_UNSUPPORTED;
}

static int mmu_lite_unmap_region(prot_domain_t* domain, uintptr_t vaddr, size_t size) {
    if (size % 4096 != 0 || vaddr % 4096 != 0) {
        return K_ERR_INVALID_ARG;
    }
    if (active_hal_pt && domain && domain->backend_state) {
        return hal_pt_unmap_range((phys_addr_t)(uintptr_t)domain->backend_state, vaddr, size);
    }
    return K_ERR_UNSUPPORTED;
}

static int mmu_lite_protect_region(prot_domain_t* domain, uintptr_t vaddr, size_t size, uint32_t flags) {
    if (size % 4096 != 0 || vaddr % 4096 != 0) {
        return K_ERR_INVALID_ARG;
    }
    if (active_hal_pt && domain && domain->backend_state) {
        return hal_pt_protect_range((phys_addr_t)(uintptr_t)domain->backend_state, vaddr, size, flags);
    }
    return K_ERR_UNSUPPORTED;
}

static int mmu_lite_query_region(prot_domain_t* domain, uintptr_t vaddr, uintptr_t* paddr, uint32_t* flags) {
    if (vaddr % 4096 != 0) {
        return K_ERR_INVALID_ARG;
    }
    if (active_hal_pt && domain && domain->backend_state) {
        size_t mapped_size;
        phys_addr_t pa;
        int res = hal_pt_query_mapping((phys_addr_t)(uintptr_t)domain->backend_state, vaddr, &pa, &mapped_size, flags);
        if (res == 0 && paddr) {
            *paddr = (uintptr_t)pa;
        }
        return res;
    }
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
#define MAX_MPU_DOMAINS_REGIONS 16

typedef struct {
    uintptr_t vaddr;
    uintptr_t paddr;
    size_t size;
    uint32_t flags;
    int region_id;
    bool active;
} mpu_domain_region_t;

typedef struct {
    mpu_domain_region_t regions[MAX_MPU_DOMAINS_REGIONS];
    int count;
} mpu_domain_state_t;

static int mpu_create(prot_domain_t** out_domain) {
    if (!active_hal_mpu || !active_hal_mpu->program_region) {
        return K_ERR_UNSUPPORTED;
    }
    prot_domain_t* domain = (prot_domain_t*)kmalloc(sizeof(prot_domain_t));
    if (!domain) return K_ERR_NO_MEMORY;

    mpu_domain_state_t* state = (mpu_domain_state_t*)kmalloc(sizeof(mpu_domain_state_t));
    if (!state) {
        kfree(domain);
        return K_ERR_NO_MEMORY;
    }
    for (int i = 0; i < MAX_MPU_DOMAINS_REGIONS; i++) {
        state->regions[i].active = false;
    }
    state->count = 0;

    domain->mode = PROT_MODE_MPU_ONLY;
    domain->ops = &mpu_backend_ops;
    domain->backend_state = state;
    *out_domain = domain;
    return K_OK;
}

static void mpu_destroy(prot_domain_t* domain) {
    if (domain) {
        if (domain->backend_state) {
            mpu_domain_state_t* state = (mpu_domain_state_t*)domain->backend_state;
            if (active_hal_mpu && active_hal_mpu->disable_region) {
                for (int i = 0; i < MAX_MPU_DOMAINS_REGIONS; i++) {
                    if (state->regions[i].active) {
                        active_hal_mpu->disable_region(state->regions[i].region_id);
                    }
                }
            }
            kfree(state);
        }
        kfree(domain);
    }
}

static void mpu_activate(prot_domain_t* domain) {
    if (!domain || !domain->backend_state) return;
    if (active_hal_mpu && active_hal_mpu->program_region && active_hal_mpu->disable_region) {
        mpu_domain_state_t* state = (mpu_domain_state_t*)domain->backend_state;
        for (int i = 0; i < MAX_MPU_DOMAINS_REGIONS; i++) {
            if (state->regions[i].active) {
                active_hal_mpu->program_region(state->regions[i].region_id, state->regions[i].paddr, state->regions[i].size, state->regions[i].flags);
            } else {
                active_hal_mpu->disable_region(i);
            }
        }
    }
}

static int mpu_map_region(prot_domain_t* domain, uintptr_t vaddr, uintptr_t paddr, size_t size, uint32_t flags) {
    if (!domain || !domain->backend_state) return K_ERR_BAD_STATE;
    if (!active_hal_mpu || !active_hal_mpu->program_region) return K_ERR_UNSUPPORTED;

    mpu_domain_state_t* state = (mpu_domain_state_t*)domain->backend_state;

    // Query capabilities for constraints
    const hal_mpu_caps_t* caps = NULL;
    if (active_hal_mpu->get_caps) {
        caps = active_hal_mpu->get_caps();
    }

    if (caps) {
        if (state->count >= (int)caps->max_regions) {
            return K_ERR_NO_RESOURCES;
        }
        if (caps->requires_power_of_two_size) {
            if (size == 0 || (size & (size - 1)) != 0) {
                return K_ERR_INVALID_ARG;
            }
        }
        if (size < caps->min_region_alignment || (vaddr % caps->min_region_alignment) != 0 || (paddr % caps->min_region_alignment) != 0) {
            return K_ERR_INVALID_ARG;
        }
    }

    // Check overlaps in our tracked state
    uintptr_t vend = vaddr + size;
    for (int i = 0; i < MAX_MPU_DOMAINS_REGIONS; i++) {
        if (state->regions[i].active) {
            uintptr_t r_vstart = state->regions[i].vaddr;
            uintptr_t r_vend = r_vstart + state->regions[i].size;
            if (!(vend <= r_vstart || vaddr >= r_vend)) {
                return K_ERR_VM_ALREADY_MAPPED;
            }
        }
    }

    // Find free slot
    int slot = -1;
    for (int i = 0; i < MAX_MPU_DOMAINS_REGIONS; i++) {
        if (!state->regions[i].active) {
            slot = i;
            break;
        }
    }
    if (slot == -1) return K_ERR_NO_RESOURCES;

    // Call active MPU backend
    int ret = active_hal_mpu->program_region(slot, paddr, size, flags);
    if (ret != K_OK) return ret;

    state->regions[slot].vaddr = vaddr;
    state->regions[slot].paddr = paddr;
    state->regions[slot].size = size;
    state->regions[slot].flags = flags;
    state->regions[slot].region_id = slot;
    state->regions[slot].active = true;
    state->count++;

    return K_OK;
}

static int mpu_unmap_region(prot_domain_t* domain, uintptr_t vaddr, size_t size) {
    if (!domain || !domain->backend_state) return K_ERR_BAD_STATE;
    if (!active_hal_mpu || !active_hal_mpu->disable_region) return K_ERR_UNSUPPORTED;

    mpu_domain_state_t* state = (mpu_domain_state_t*)domain->backend_state;
    for (int i = 0; i < MAX_MPU_DOMAINS_REGIONS; i++) {
        if (state->regions[i].active && state->regions[i].vaddr == vaddr && state->regions[i].size == size) {
            int ret = active_hal_mpu->disable_region(state->regions[i].region_id);
            if (ret != K_OK) return ret;

            state->regions[i].active = false;
            state->count--;
            return K_OK;
        }
    }
    return K_ERR_NOT_FOUND;
}

static int mpu_protect_region(prot_domain_t* domain, uintptr_t vaddr, size_t size, uint32_t flags) {
    if (!domain || !domain->backend_state) return K_ERR_BAD_STATE;
    if (!active_hal_mpu || !active_hal_mpu->program_region) return K_ERR_UNSUPPORTED;

    mpu_domain_state_t* state = (mpu_domain_state_t*)domain->backend_state;
    for (int i = 0; i < MAX_MPU_DOMAINS_REGIONS; i++) {
        if (state->regions[i].active && state->regions[i].vaddr == vaddr && state->regions[i].size == size) {
            int ret = active_hal_mpu->program_region(state->regions[i].region_id, state->regions[i].paddr, state->regions[i].size, flags);
            if (ret != K_OK) return ret;

            state->regions[i].flags = flags;
            return K_OK;
        }
    }
    return K_ERR_NOT_FOUND;
}

static int mpu_query_region(prot_domain_t* domain, uintptr_t vaddr, uintptr_t* paddr, uint32_t* flags) {
    if (!domain || !domain->backend_state) return K_ERR_BAD_STATE;

    mpu_domain_state_t* state = (mpu_domain_state_t*)domain->backend_state;
    for (int i = 0; i < MAX_MPU_DOMAINS_REGIONS; i++) {
        if (state->regions[i].active) {
            uintptr_t r_vstart = state->regions[i].vaddr;
            uintptr_t r_vend = r_vstart + state->regions[i].size;
            if (vaddr >= r_vstart && vaddr < r_vend) {
                if (paddr) {
                    *paddr = state->regions[i].paddr + (vaddr - r_vstart);
                }
                if (flags) {
                    *flags = state->regions[i].flags;
                }
                return K_OK;
            }
        }
    }
    return K_ERR_NOT_FOUND;
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

int prot_domain_create(prot_domain_t** out_domain) {
    if (!out_domain) return K_ERR_INVALID_ARG;
    if (!active_backend || !active_backend->create) {
        return K_ERR_UNSUPPORTED;
    }
    return active_backend->create(out_domain);
}

void prot_domain_destroy(prot_domain_t* domain) {
    if (domain && domain->ops && domain->ops->destroy) {
        domain->ops->destroy(domain);
    }
}

void prot_domain_activate(prot_domain_t* domain) {
    if (domain && domain->ops && domain->ops->activate) {
        domain->ops->activate(domain);
    }
}

int prot_domain_map_region(prot_domain_t* domain, uintptr_t vaddr, uintptr_t paddr, size_t size, uint32_t flags) {
    if (!domain || !domain->ops || !domain->ops->map_region) return K_ERR_UNSUPPORTED;
    return domain->ops->map_region(domain, vaddr, paddr, size, flags);
}

int prot_domain_unmap_region(prot_domain_t* domain, uintptr_t vaddr, size_t size) {
    if (!domain || !domain->ops || !domain->ops->unmap_region) return K_ERR_UNSUPPORTED;
    return domain->ops->unmap_region(domain, vaddr, size);
}

int prot_domain_protect_region(prot_domain_t* domain, uintptr_t vaddr, size_t size, uint32_t flags) {
    if (!domain || !domain->ops || !domain->ops->protect_region) return K_ERR_UNSUPPORTED;
    return domain->ops->protect_region(domain, vaddr, size, flags);
}

int prot_domain_query_region(prot_domain_t* domain, uintptr_t vaddr, uintptr_t* paddr, uint32_t* flags) {
    if (!domain || !domain->ops || !domain->ops->query_region) return K_ERR_UNSUPPORTED;
    return domain->ops->query_region(domain, vaddr, paddr, flags);
}
