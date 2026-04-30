#ifndef BHARAT_HAL_TLB_H
#define BHARAT_HAL_TLB_H

#include <stdint.h>
#include <stdbool.h>
#include "../../include/mm.h"

// TLB Invalidation Scopes
typedef enum {
    TLB_SCOPE_PAGE   = 1,
    TLB_SCOPE_RANGE  = 2,
    TLB_SCOPE_ASPACE = 3,
    TLB_SCOPE_ALL    = 4
} tlb_scope_t;

typedef struct hal_tlb_caps {
    bool supports_page_flush;
    bool supports_range_flush;
    bool supports_aspace_flush;
    bool supports_all_flush;
    bool supports_remote_targeted_flush;
    bool supports_broadcast_flush;
    bool supports_asid_selective_flush;
    bool supports_lazy_generation_model;
} hal_tlb_caps_t;

// Architecture-neutral TLB Invalidation API
typedef struct hal_tlb_ops {
    const hal_tlb_caps_t *caps;
    // Local Core TLB Operations
    void (*flush_page_local)(virt_addr_t vaddr);
    void (*flush_range_local)(virt_addr_t start, size_t len);
    void (*flush_all_local)(void);
    void (*flush_asid_local)(uint16_t asid);

    // Remote Core / Broadcast TLB Operations
    // If not hardware broadcast, this will rely on URPC IPIs in the cross-core layer.
    // The HAL implementation can do direct broadcast if the ISA supports it.
    void (*flush_page_remote)(uint16_t target_core, uint16_t asid, virt_addr_t vaddr);
    void (*flush_range_remote)(uint16_t target_core, uint16_t asid, virt_addr_t start, size_t len);
    void (*flush_all_remote)(uint16_t target_core, uint16_t asid);
    void (*flush_page_broadcast)(uint16_t asid, virt_addr_t vaddr);
    void (*flush_range_broadcast)(uint16_t asid, virt_addr_t start, size_t len);
    void (*flush_all_broadcast)(uint16_t asid);
} hal_tlb_ops_t;

extern hal_tlb_ops_t *active_hal_tlb;

// Architecture-neutral synchronous shootdown coordinator API
void hal_tlb_invalidate_page(address_space_t *aspace, virt_addr_t va);
void hal_tlb_invalidate_range(address_space_t *aspace, virt_addr_t start, size_t len);
void hal_tlb_invalidate_aspace(address_space_t *aspace);
void hal_tlb_invalidate_all(void);

void hal_tlb_init(void);
const hal_tlb_caps_t *hal_tlb_caps(void);

// KERN-P0-002: Bounded TLB Shootdown HAL Wrappers
void hal_tlb_invalidate_local_page(virt_addr_t va);
void hal_tlb_invalidate_local_range(virt_addr_t start, size_t len);
void hal_tlb_invalidate_local_aspace(uint64_t aspace_id);

#endif // BHARAT_HAL_TLB_H
