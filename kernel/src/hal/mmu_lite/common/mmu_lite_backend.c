#include "mm/prot_domain.h"
#include "slab.h"
#include <stddef.h>

#define ERR_NOT_SUPPORTED -1
#define MAX_LITE_REGIONS 64

// MMU Lite backend
// Acts as a structured hardware boundary: eager mappings, constrained features, flat contiguous limits.

typedef struct {
    uintptr_t vaddr;
    uintptr_t paddr;
    size_t size;
    uint32_t flags;
    bool valid;
} mmu_lite_region_t;

typedef struct {
    mmu_lite_region_t regions[MAX_LITE_REGIONS];
    int region_count;
    void* hardware_root_pt;
} mmu_lite_backend_state_t;

extern void* lite_hw_pt_create(void);
extern void lite_hw_pt_destroy(void* pt);
extern void lite_hw_pt_activate(void* pt);
extern int lite_hw_map(void* pt, uintptr_t v, uintptr_t p, size_t s, uint32_t f);
extern int lite_hw_unmap(void* pt, uintptr_t v, size_t s);

static prot_domain_t* mmu_lite_create(void) {
    prot_domain_t* domain = (prot_domain_t*)kmalloc(sizeof(prot_domain_t));
    if (!domain) return NULL;

    mmu_lite_backend_state_t* state = (mmu_lite_backend_state_t*)kmalloc(sizeof(mmu_lite_backend_state_t));
    if (!state) {
        kfree(domain);
        return NULL;
    }

    for (int i = 0; i < MAX_LITE_REGIONS; i++) {
        state->regions[i].valid = false;
    }
    state->region_count = 0;
    state->hardware_root_pt = NULL; // We'll stub this out for now

    domain->mode = PROT_MODE_MMU_LITE;
    domain->backend_state = state;
    return domain;
}

static void mmu_lite_destroy(prot_domain_t* domain) {
    if (!domain) return;
    if (domain->backend_state) {
        // lite_hw_pt_destroy(((mmu_lite_backend_state_t*)domain->backend_state)->hardware_root_pt);
        kfree(domain->backend_state);
    }
    kfree(domain);
}

static void mmu_lite_activate(prot_domain_t* domain) {
    if (!domain || domain->mode != PROT_MODE_MMU_LITE) return;

    // mmu_lite_backend_state_t* state = (mmu_lite_backend_state_t*)domain->backend_state;
    // lite_hw_pt_activate(state->hardware_root_pt);
}

static int mmu_lite_map_region(prot_domain_t* domain, uintptr_t vaddr, uintptr_t paddr, size_t size, uint32_t flags) {
    if (!domain || domain->mode != PROT_MODE_MMU_LITE) return ERR_NOT_SUPPORTED;

    // Explicitly reject non-contiguous/sparse mapping
    // E.g., if we were to enforce flat mappings:
    // if (vaddr != paddr) return ERR_NOT_SUPPORTED;

    mmu_lite_backend_state_t* state = (mmu_lite_backend_state_t*)domain->backend_state;
    if (state->region_count >= MAX_LITE_REGIONS) return -1; // Exhausted limits

    for (int i = 0; i < MAX_LITE_REGIONS; i++) {
        if (!state->regions[i].valid) {
            state->regions[i].vaddr = vaddr;
            state->regions[i].paddr = paddr;
            state->regions[i].size = size;
            state->regions[i].flags = flags;
            state->regions[i].valid = true;
            state->region_count++;

            // return lite_hw_map(state->hardware_root_pt, vaddr, paddr, size, flags);
            return 0;
        }
    }
    return -1;
}

static int mmu_lite_unmap_region(prot_domain_t* domain, uintptr_t vaddr, size_t size) {
    if (!domain || domain->mode != PROT_MODE_MMU_LITE) return ERR_NOT_SUPPORTED;

    mmu_lite_backend_state_t* state = (mmu_lite_backend_state_t*)domain->backend_state;

    for (int i = 0; i < MAX_LITE_REGIONS; i++) {
        // Demand unmapping exact regions
        if (state->regions[i].valid && state->regions[i].vaddr == vaddr && state->regions[i].size == size) {
            state->regions[i].valid = false;
            state->region_count--;
            // return lite_hw_unmap(state->hardware_root_pt, vaddr, size);
            return 0;
        }
    }
    return -1;
}

static int mmu_lite_protect_region(prot_domain_t* domain, uintptr_t vaddr, size_t size, uint32_t flags) {
    if (!domain || domain->mode != PROT_MODE_MMU_LITE) return ERR_NOT_SUPPORTED;

    mmu_lite_backend_state_t* state = (mmu_lite_backend_state_t*)domain->backend_state;
    for (int i = 0; i < MAX_LITE_REGIONS; i++) {
        if (state->regions[i].valid && state->regions[i].vaddr == vaddr && state->regions[i].size == size) {
            state->regions[i].flags = flags;
            // update hardware here
            return 0;
        }
    }
    return -1;
}

static int mmu_lite_query_region(prot_domain_t* domain, uintptr_t vaddr, uintptr_t* paddr, uint32_t* flags) {
    if (!domain || domain->mode != PROT_MODE_MMU_LITE) return ERR_NOT_SUPPORTED;

    mmu_lite_backend_state_t* state = (mmu_lite_backend_state_t*)domain->backend_state;
    for (int i = 0; i < MAX_LITE_REGIONS; i++) {
        if (!state->regions[i].valid) continue;
        uintptr_t r_base = state->regions[i].vaddr;
        uintptr_t r_end = r_base + state->regions[i].size;

        if (vaddr >= r_base && vaddr < r_end) {
            if (paddr) *paddr = state->regions[i].paddr + (vaddr - r_base);
            if (flags) *flags = state->regions[i].flags;
            return 0;
        }
    }
    return -1;
}

prot_domain_ops_t mmu_lite_ops_common = {
    .create = mmu_lite_create,
    .destroy = mmu_lite_destroy,
    .activate = mmu_lite_activate,
    .map_region = mmu_lite_map_region,
    .unmap_region = mmu_lite_unmap_region,
    .protect_region = mmu_lite_protect_region,
    .query_region = mmu_lite_query_region,
};
