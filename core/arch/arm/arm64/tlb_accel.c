#include "arch/arch_tlb_accel.h"

void arch_tlb_fast_ctx_init(void) {
    // ARM64 architecture natively supports ASID-tagged translation regime (VMSAv8-64)
}

bool arch_tlb_fast_ctx_supported(void) {
    return true;
}

void arch_tlb_prepare_full_flush_cr3(uintptr_t *cr3) {
    (void)cr3;
}

bool arch_tlb_flush_asid(uint16_t asid) {
    // Invalidate all TLB entries matching ASID across inner shareable domain
    asm volatile(
        "tlbi aside1is, %0\n"
        "dsb ish\n"
        "isb\n"
        :: "r"((uint64_t)asid << 48) : "memory"
    );
    return true;
}

bool arch_tlb_flush_addr_asid(uintptr_t vaddr, uint16_t asid) {
    // Invalidate TLB entry matching VA and ASID across inner shareable domain
    uint64_t val = ((uint64_t)asid << 48) | ((vaddr >> 12) & 0x0000FFFFFFFFFFFFULL);
    asm volatile(
        "tlbi vae1is, %0\n"
        "dsb ish\n"
        "isb\n"
        :: "r"(val) : "memory"
    );
    return true;
}
