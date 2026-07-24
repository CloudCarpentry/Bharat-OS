#ifndef BHARAT_ARCH_TLB_ACCEL_H
#define BHARAT_ARCH_TLB_ACCEL_H

#include <stdbool.h>
#include <stdint.h>

void arch_tlb_fast_ctx_init(void);
bool arch_tlb_fast_ctx_supported(void);
void arch_tlb_prepare_full_flush_cr3(uintptr_t *cr3);
bool arch_tlb_flush_asid(uint16_t asid);
bool arch_tlb_flush_addr_asid(uintptr_t vaddr, uint16_t asid);

#endif
