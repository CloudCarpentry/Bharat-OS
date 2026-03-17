#ifndef BHARAT_HAL_TLB_H
#define BHARAT_HAL_TLB_H

#include <stdint.h>
#include "../../include/mm.h"

// Architecture-neutral TLB Invalidation API
typedef struct hal_tlb_ops {
    // Local Core TLB Operations
    void (*flush_page_local)(virt_addr_t vaddr);
    void (*flush_all_local)(void);
    void (*flush_asid_local)(uint16_t asid);

    // Remote Core / Broadcast TLB Operations (Architecture/Platform Specific Broadcast)
    // If not hardware broadcast, this will rely on URPC IPIs in the cross-core layer.
    // The HAL implementation can do direct broadcast if the ISA supports it (e.g. RISC-V sfence.vma, ARM64 tlbi).
    void (*flush_page_remote)(uint16_t target_core, uint16_t asid, virt_addr_t vaddr);
    void (*flush_all_remote)(uint16_t target_core, uint16_t asid);
    void (*flush_page_broadcast)(uint16_t asid, virt_addr_t vaddr);
    void (*flush_all_broadcast)(uint16_t asid);
} hal_tlb_ops_t;

extern hal_tlb_ops_t *active_hal_tlb;

void hal_tlb_init(void);

#endif // BHARAT_HAL_TLB_H
