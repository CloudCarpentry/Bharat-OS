#include "../../../../include/mm/prot_domain.h"
#include "../../../../include/console/console_core.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define MAX_MPU_REGIONS 16
#define ERR_NOT_SUPPORTED -1

typedef struct {
    uintptr_t base;
    size_t size;
    uint32_t flags;
    bool valid;
} mpu_region_t;

typedef struct {
    mpu_region_t regions[MAX_MPU_REGIONS];
    int region_count;
} mpu_backend_state_t;

// TODO: Needs refactor: #include directive placed mid-file for dependency/order compatibility.
#include "../../../../include/slab.h"

static prot_domain_t* arm32_mpu_create(void) {
    prot_domain_t* domain = (prot_domain_t*)kmalloc(sizeof(prot_domain_t)); // Using basic alloc for demo
    if (!domain) return NULL;

    mpu_backend_state_t* state = (mpu_backend_state_t*)kmalloc(sizeof(mpu_backend_state_t));
    if (!state) {
        kfree(domain);
        return NULL;
    }

    for (int i = 0; i < MAX_MPU_REGIONS; i++) {
        state->regions[i].valid = false;
    }
    state->region_count = 0;

    domain->mode = PROT_MODE_MPU_ONLY;
    domain->backend_state = state;
    return domain;
}

static void arm32_mpu_destroy(prot_domain_t* domain) {
    if (!domain) return;
    if (domain->backend_state) {
        kfree(domain->backend_state);
    }
    kfree(domain);
}

static void arm32_mpu_activate(prot_domain_t* domain) {
    if (!domain || domain->mode != PROT_MODE_MPU_ONLY) return;

    mpu_backend_state_t* state = (mpu_backend_state_t*)domain->backend_state;
    // Simulated region reprogramming on context switch
    for (int i = 0; i < MAX_MPU_REGIONS; i++) {
        if (state->regions[i].valid) {
            // e.g. hal_mpu_region_set(i, base, size, flags)
            // console_write_raw("Programmed MPU Region\n", 22);
        } else {
            // hal_mpu_region_clear(i)
        }
    }
}

static bool check_overlap(mpu_backend_state_t* state, uintptr_t base, size_t size) {
    uintptr_t end = base + size;
    for (int i = 0; i < MAX_MPU_REGIONS; i++) {
        if (!state->regions[i].valid) continue;
        uintptr_t r_base = state->regions[i].base;
        uintptr_t r_end = r_base + state->regions[i].size;

        if (base < r_end && end > r_base) {
            return true; // Overlap detected
        }
    }
    return false;
}

static int arm32_mpu_map_region(prot_domain_t* domain, uintptr_t vaddr, uintptr_t paddr, size_t size, uint32_t flags) {
    if (!domain || domain->mode != PROT_MODE_MPU_ONLY) return ERR_NOT_SUPPORTED;
    if (vaddr != paddr) return ERR_NOT_SUPPORTED; // No sparse mappings, identity only for MPU

    mpu_backend_state_t* state = (mpu_backend_state_t*)domain->backend_state;
    if (state->region_count >= MAX_MPU_REGIONS) return -1; // Region exhaustion

    if (check_overlap(state, paddr, size)) return -1; // Overlap rejection

    for (int i = 0; i < MAX_MPU_REGIONS; i++) {
        if (!state->regions[i].valid) {
            state->regions[i].base = paddr;
            state->regions[i].size = size;
            state->regions[i].flags = flags;
            state->regions[i].valid = true;
            state->region_count++;
            return 0;
        }
    }
    return -1;
}

static int arm32_mpu_unmap_region(prot_domain_t* domain, uintptr_t vaddr, size_t size) {
    if (!domain || domain->mode != PROT_MODE_MPU_ONLY) return ERR_NOT_SUPPORTED;

    mpu_backend_state_t* state = (mpu_backend_state_t*)domain->backend_state;

    for (int i = 0; i < MAX_MPU_REGIONS; i++) {
        if (state->regions[i].valid && state->regions[i].base == vaddr && state->regions[i].size == size) {
            state->regions[i].valid = false;
            state->region_count--;
            return 0;
        }
    }
    return -1; // Region not found exactly
}

static int arm32_mpu_protect_region(prot_domain_t* domain, uintptr_t vaddr, size_t size, uint32_t flags) {
    if (!domain || domain->mode != PROT_MODE_MPU_ONLY) return ERR_NOT_SUPPORTED;

    mpu_backend_state_t* state = (mpu_backend_state_t*)domain->backend_state;
    for (int i = 0; i < MAX_MPU_REGIONS; i++) {
        if (state->regions[i].valid && state->regions[i].base == vaddr && state->regions[i].size == size) {
            state->regions[i].flags = flags;
            return 0;
        }
    }
    return -1;
}

static int arm32_mpu_query_region(prot_domain_t* domain, uintptr_t vaddr, uintptr_t* paddr, uint32_t* flags) {
    if (!domain || domain->mode != PROT_MODE_MPU_ONLY) return ERR_NOT_SUPPORTED;

    mpu_backend_state_t* state = (mpu_backend_state_t*)domain->backend_state;
    for (int i = 0; i < MAX_MPU_REGIONS; i++) {
        if (!state->regions[i].valid) continue;
        uintptr_t r_base = state->regions[i].base;
        uintptr_t r_end = r_base + state->regions[i].size;

        if (vaddr >= r_base && vaddr < r_end) {
            if (paddr) *paddr = vaddr; // Identity
            if (flags) *flags = state->regions[i].flags;
            return 0;
        }
    }
    return -1;
}

prot_domain_ops_t mpu_only_ops_arm32 = {
    .create = arm32_mpu_create,
    .destroy = arm32_mpu_destroy,
    .activate = arm32_mpu_activate,
    .map_region = arm32_mpu_map_region,
    .unmap_region = arm32_mpu_unmap_region,
    .protect_region = arm32_mpu_protect_region,
    .query_region = arm32_mpu_query_region,
};
