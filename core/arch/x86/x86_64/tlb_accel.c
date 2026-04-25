#include "arch/arch_tlb_accel.h"
#include "arch/arch_cpu_caps.h"

static bool g_x86_pcid_supported = false;
static bool g_x86_invpcid_supported = false;
static bool g_x86_fast_tlb_ctx = false;

void arch_tlb_fast_ctx_init(void) {
    g_x86_pcid_supported = arch_cpu_has_system_all(ARCH_CPU_FEAT_X86_PCID);
    g_x86_invpcid_supported = arch_cpu_has_system_all(ARCH_CPU_FEAT_X86_INVPCID);
    g_x86_fast_tlb_ctx = arch_cpu_has_system_all(ARCH_CPU_FEAT_COMMON_FAST_TLB_CTX);
}

bool arch_tlb_fast_ctx_supported(void) {
    return g_x86_fast_tlb_ctx;
}

void arch_tlb_prepare_full_flush_cr3(uintptr_t *cr3) {
    if (!cr3) {
        return;
    }

    if (g_x86_pcid_supported) {
        *cr3 &= ~(1ULL << 63);
    }
}

bool arch_tlb_flush_asid(uint16_t asid) {
    if (!(g_x86_fast_tlb_ctx && g_x86_invpcid_supported)) {
        return false;
    }

    struct {
        uint64_t pcid;
        uint64_t addr;
    } desc = { .pcid = (uint64_t)(asid & 0x0FFFu), .addr = 0 };

    __asm__ volatile("invpcid %0, %1" :: "m"(desc), "r"(1ULL) : "memory");
    return true;
}

bool arch_tlb_flush_addr_asid(uintptr_t vaddr, uint16_t asid) {
    if (!(g_x86_fast_tlb_ctx && g_x86_invpcid_supported)) {
        return false;
    }

    struct {
        uint64_t pcid;
        uint64_t addr;
    } desc = { .pcid = (uint64_t)(asid & 0x0FFFu), .addr = (uint64_t)vaddr };

    __asm__ volatile("invpcid %0, %1" :: "m"(desc), "r"(0ULL) : "memory");
    return true;
}
