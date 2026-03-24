#include "mm/prot_domain.h"
#include "../../include/mm/prot_domain.h"
#include "../../include/hal/hal_pt.h"
#include "../../include/arch/arch_caps.h"
#include "hal/hal_mm.h"
#include "console/console_core.h"
#include <stddef.h>

// Forward declaration of the generic wrapper ops
static prot_domain_ops_t generic_backend_ops;
static prot_domain_ops_t* active_backend = NULL;
static prot_mode_t active_mode = PROT_MODE_NONE;

// Generic wrapper implementations relying on hal_pt and hal_mm capabilities
static prot_domain_t* generic_create(void) {
    return NULL; // Stubbed as prot_domain_t is legacy/transitional. VMM uses aspace directly.
}

static void generic_destroy(prot_domain_t* domain) {
    (void)domain;
}

static void generic_activate(prot_domain_t* domain) {
    (void)domain;
}

static int generic_map_region(prot_domain_t* domain, uintptr_t vaddr, uintptr_t paddr, size_t size, uint32_t flags) {
    (void)domain;
    if (active_hal_pt && active_hal_pt->map_range) {
        return active_hal_pt->map_range(0 /* root_pt handled by aspace */, vaddr, paddr, size, flags);
    }
    return -1;
}

static int generic_unmap_region(prot_domain_t* domain, uintptr_t vaddr, size_t size) {
    (void)domain;
    if (active_hal_pt && active_hal_pt->unmap_range) {
        return active_hal_pt->unmap_range(0 /* root_pt handled by aspace */, vaddr, size);
    }
    return -1;
}

static int generic_protect_region(prot_domain_t* domain, uintptr_t vaddr, size_t size, uint32_t flags) {
    (void)domain;
    if (active_hal_pt && active_hal_pt->protect_range) {
        return active_hal_pt->protect_range(0 /* root_pt handled by aspace */, vaddr, size, flags);
    }
    return -1;
}

static int generic_query_region(prot_domain_t* domain, uintptr_t vaddr, uintptr_t* paddr, uint32_t* flags) {
    (void)domain;
    if (active_hal_pt && active_hal_pt->query_mapping) {
        size_t mapped_size;
        return active_hal_pt->query_mapping(0 /* root_pt handled by aspace */, vaddr, paddr, &mapped_size, flags);
    }
    return -1;
}

static prot_domain_ops_t generic_backend_ops = {
    .create = generic_create,
    .destroy = generic_destroy,
    .activate = generic_activate,
    .map_region = generic_map_region,
    .unmap_region = generic_unmap_region,
    .protect_region = generic_protect_region,
    .query_region = generic_query_region,
};

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
        active_backend = &generic_backend_ops;
    } else if (backend_caps.kind == HAL_MM_BACKEND_MMU_LITE) {
        active_mode = PROT_MODE_MMU_LITE;
        active_backend = &generic_backend_ops;
    } else if (backend_caps.kind == HAL_MM_BACKEND_MPU_ONLY) {
        active_mode = PROT_MODE_MPU_ONLY;
        active_backend = &generic_backend_ops;
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
