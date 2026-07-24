#include "arch/arch_tlb_accel.h"

__attribute__((weak)) void arch_tlb_fast_ctx_init(void) {
}

__attribute__((weak)) bool arch_tlb_fast_ctx_supported(void) {
    return false;
}

__attribute__((weak)) void arch_tlb_prepare_full_flush_cr3(uintptr_t *cr3) {
    (void)cr3;
}

__attribute__((weak)) bool arch_tlb_flush_asid(uint16_t asid) {
    (void)asid;
    return false;
}

__attribute__((weak)) bool arch_tlb_flush_addr_asid(uintptr_t vaddr, uint16_t asid) {
    (void)vaddr;
    (void)asid;
    return false;
}
