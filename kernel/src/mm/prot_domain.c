#include "mm/prot_domain.h"
#include "../../include/mm/prot_domain.h"
#include "../../include/hal/hal_pt.h"
#include "../../include/arch/arch_caps.h"
#include "hal/hal_mm.h"
#include "console/console_core.h"
#include <stddef.h>

static prot_domain_ops_t* active_backend = NULL;
static prot_mode_t active_mode = PROT_MODE_NONE;

#if defined(__x86_64__)
extern prot_domain_ops_t mmu_full_ops_x86_64;
#elif defined(__aarch64__)
extern prot_domain_ops_t mmu_full_ops_arm64;
#elif defined(__riscv) && __riscv_xlen == 64
extern prot_domain_ops_t mmu_full_ops_riscv64;
#elif defined(__arm__)
extern prot_domain_ops_t mpu_only_ops_arm32;
#endif

extern prot_domain_ops_t prot_none_ops;
extern prot_domain_ops_t mmu_lite_ops_common;

void prot_domain_init(void) {
    hal_mm_backend_caps_t backend_caps;
    hal_mm_backend_caps(&backend_caps);

    if (backend_caps.kind == HAL_MM_BACKEND_MMU_FULL) {
        active_mode = PROT_MODE_MMU_FULL;
#if defined(__x86_64__)
        active_backend = &mmu_full_ops_x86_64;
#elif defined(__aarch64__)
        active_backend = &mmu_full_ops_arm64;
#elif defined(__riscv) && __riscv_xlen == 64
        active_backend = &mmu_full_ops_riscv64;
#else
        active_backend = &prot_none_ops;
#endif
    } else if (backend_caps.kind == HAL_MM_BACKEND_MMU_LITE) {
        active_mode = PROT_MODE_MMU_LITE;
        active_backend = &mmu_lite_ops_common;
    } else if (backend_caps.kind == HAL_MM_BACKEND_MPU_ONLY) {
        active_mode = PROT_MODE_MPU_ONLY;
#if defined(__arm__)
        active_backend = &mpu_only_ops_arm32;
#else
        active_backend = &prot_none_ops;
#endif
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
