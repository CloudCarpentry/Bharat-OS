#include "arch/arch_cpu_caps.h"
#include "../../common/cpu_caps_state.h"
#include <stdint.h>

extern uint32_t hal_cpu_get_id(void);

static inline void x86_cpuid(uint32_t leaf, uint32_t subleaf, uint32_t *eax, uint32_t *ebx, uint32_t *ecx, uint32_t *edx) {
    __asm__ volatile("cpuid"
        : "=a"(*eax), "=b"(*ebx), "=c"(*ecx), "=d"(*edx)
        : "a"(leaf), "c"(subleaf)
    );
}

static inline uint64_t x86_read_xcr0(void) {
    uint32_t eax, edx;
    __asm__ volatile("xgetbv" : "=a"(eax), "=d"(edx) : "c"(0));
    return ((uint64_t)edx << 32) | eax;
}

static bool x86_vector_policy_allows(void) {
#ifdef BHARAT_X86_DISABLE_VECTOR_CONTEXT
    return false;
#else
    return true;
#endif
}

static void x86_probe_caps(arch_cpu_caps_record_t *caps) {
    arch_cpu_caps_zero(&caps->raw);
    arch_cpu_caps_zero(&caps->usable);

    uint32_t eax, ebx, ecx, edx;
    uint32_t max_leaf = 0;

    x86_cpuid(0, 0, &max_leaf, &ebx, &ecx, &edx);
    if (max_leaf < 1) {
        return;
    }

    x86_cpuid(1, 0, &eax, &ebx, &ecx, &edx);

    const bool xsave_hw = (ecx & (1u << 26)) != 0;
    const bool osxsave = (ecx & (1u << 27)) != 0;
    const bool avx_hw = (ecx & (1u << 28)) != 0;
    const bool fma_hw = (ecx & (1u << 12)) != 0;

    if (ecx & (1u << 1)) {
        arch_cpu_caps_set(&caps->raw, ARCH_CPU_FEAT_X86_PCLMUL);
        arch_cpu_caps_set(&caps->usable, ARCH_CPU_FEAT_X86_PCLMUL);
    }
    if (ecx & (1u << 25)) {
        arch_cpu_caps_set(&caps->raw, ARCH_CPU_FEAT_X86_AES);
        arch_cpu_caps_set(&caps->raw, ARCH_CPU_FEAT_COMMON_AES);
        arch_cpu_caps_set(&caps->usable, ARCH_CPU_FEAT_X86_AES);
        arch_cpu_caps_set(&caps->usable, ARCH_CPU_FEAT_COMMON_AES);
    }

    // PCID is reported in leaf 1 ECX[17].
    if (ecx & (1u << 17)) {
        arch_cpu_caps_set(&caps->raw, ARCH_CPU_FEAT_X86_PCID);
        arch_cpu_caps_set(&caps->usable, ARCH_CPU_FEAT_X86_PCID);
    }

    if (avx_hw) {
        arch_cpu_caps_set(&caps->raw, ARCH_CPU_FEAT_X86_AVX);
    }
    if (fma_hw) {
        arch_cpu_caps_set(&caps->raw, ARCH_CPU_FEAT_X86_FMA);
    }

    bool avx2_hw = false;
    bool invpcid_hw = false;

    if (max_leaf >= 7) {
        x86_cpuid(7, 0, &eax, &ebx, &ecx, &edx);

        if (ebx & (1u << 0)) {
            arch_cpu_caps_set(&caps->raw, ARCH_CPU_FEAT_X86_FSGSBASE);
            arch_cpu_caps_set(&caps->usable, ARCH_CPU_FEAT_X86_FSGSBASE);
        }
        if (ebx & (1u << 10)) {
            invpcid_hw = true;
            arch_cpu_caps_set(&caps->raw, ARCH_CPU_FEAT_X86_INVPCID);
            arch_cpu_caps_set(&caps->usable, ARCH_CPU_FEAT_X86_INVPCID);
        }
        if (ebx & (1u << 29)) {
            arch_cpu_caps_set(&caps->raw, ARCH_CPU_FEAT_X86_SHA);
            arch_cpu_caps_set(&caps->raw, ARCH_CPU_FEAT_COMMON_SHA);
            arch_cpu_caps_set(&caps->usable, ARCH_CPU_FEAT_X86_SHA);
            arch_cpu_caps_set(&caps->usable, ARCH_CPU_FEAT_COMMON_SHA);
        }
        if (ebx & (1u << 5)) {
            avx2_hw = true;
            arch_cpu_caps_set(&caps->raw, ARCH_CPU_FEAT_X86_AVX2);
        }
    }

    // Vector usable == HW detected + XSAVE plumbing + kernel context policy.
    bool vector_context_ready = false;
    if (xsave_hw && osxsave) {
        const uint64_t xcr0 = x86_read_xcr0();
        // XMM (bit 1) + YMM (bit 2) states must be enabled by OS/kernel.
        vector_context_ready = (xcr0 & 0x6u) == 0x6u;
    }

    if (avx_hw && vector_context_ready && x86_vector_policy_allows()) {
        arch_cpu_caps_set(&caps->usable, ARCH_CPU_FEAT_X86_AVX);
        arch_cpu_caps_set(&caps->usable, ARCH_CPU_FEAT_COMMON_VECTOR);

        if (fma_hw) {
            arch_cpu_caps_set(&caps->usable, ARCH_CPU_FEAT_X86_FMA);
        }

        if (avx2_hw) {
            arch_cpu_caps_set(&caps->usable, ARCH_CPU_FEAT_X86_AVX2);
        }
    }

    // Fast TLB context switch requires both tags (PCID) and selective invalidate (INVPCID).
    if (arch_cpu_caps_test(&caps->raw, ARCH_CPU_FEAT_X86_PCID) && invpcid_hw) {
        arch_cpu_caps_set(&caps->raw, ARCH_CPU_FEAT_COMMON_FAST_TLB_CTX);
    }
    if (arch_cpu_caps_test(&caps->usable, ARCH_CPU_FEAT_X86_PCID) &&
        arch_cpu_caps_test(&caps->usable, ARCH_CPU_FEAT_X86_INVPCID)) {
        arch_cpu_caps_set(&caps->usable, ARCH_CPU_FEAT_COMMON_FAST_TLB_CTX);
    }
}

extern void x86_64_init_syscall(void);

void arch_cpu_caps_init(void) {
    arch_cpu_caps_record_t boot_caps;
    x86_probe_caps(&boot_caps);
    cpu_caps_state_set_boot(&boot_caps);
#if defined(BHARAT_X86_64_ENABLE_FAST_SYSCALL)
    x86_64_init_syscall();
#endif
}

void arch_cpu_caps_init_ap(void) {
    arch_cpu_caps_record_t ap_caps;
    x86_probe_caps(&ap_caps);
    cpu_caps_state_set_ap(hal_cpu_get_id(), &ap_caps);
#if defined(BHARAT_X86_64_ENABLE_FAST_SYSCALL)
    x86_64_init_syscall();
#endif
}
