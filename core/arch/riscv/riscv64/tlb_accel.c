#include "arch/arch_tlb_accel.h"

void arch_tlb_fast_ctx_init(void) {
    // RISC-V privileged architecture supports ASID-tagged SFENCE.VMA
}

bool arch_tlb_fast_ctx_supported(void) {
    return true;
}

void arch_tlb_prepare_full_flush_cr3(uintptr_t *cr3) {
    (void)cr3;
}

bool arch_tlb_flush_asid(uint16_t asid) {
    // SFENCE.VMA with rs1=x0 (all VAs) and rs2=asid
    asm volatile(
        "sfence.vma zero, %0\n"
        :: "r"((unsigned long)asid) : "memory"
    );
    return true;
}

bool arch_tlb_flush_addr_asid(uintptr_t vaddr, uint16_t asid) {
    // SFENCE.VMA with rs1=vaddr and rs2=asid
    asm volatile(
        "sfence.vma %0, %1\n"
        :: "r"(vaddr), "r"((unsigned long)asid) : "memory"
    );
    return true;
}
