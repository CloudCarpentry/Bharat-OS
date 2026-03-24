#include "arch/arch_cpu_caps.h"
#include "../../common/cpu_caps_state.h"
#include <stdint.h>

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

static void x86_probe_caps(arch_cpu_caps_record_t *caps) {
    arch_cpu_caps_zero(&caps->raw);
    arch_cpu_caps_zero(&caps->usable);

    uint32_t eax, ebx, ecx, edx;

    // Standard Features (Leaf 1)
    x86_cpuid(1, 0, &eax, &ebx, &ecx, &edx);

    // ecx bits
    if (ecx & (1 << 1)) {
        arch_cpu_caps_set(&caps->raw, ARCH_CPU_FEAT_X86_PCLMUL);
        arch_cpu_caps_set(&caps->usable, ARCH_CPU_FEAT_X86_PCLMUL);
    }
    if (ecx & (1 << 25)) {
        arch_cpu_caps_set(&caps->raw, ARCH_CPU_FEAT_X86_AES);
        arch_cpu_caps_set(&caps->raw, ARCH_CPU_FEAT_COMMON_AES);
        arch_cpu_caps_set(&caps->usable, ARCH_CPU_FEAT_X86_AES);
        arch_cpu_caps_set(&caps->usable, ARCH_CPU_FEAT_COMMON_AES);
    }

    // osxsave check
    bool osxsave = (ecx & (1 << 27)) != 0;

    // Extended Features (Leaf 7, Subleaf 0)
    x86_cpuid(7, 0, &eax, &ebx, &ecx, &edx);

    // ebx bits
    if (ebx & (1 << 0)) {
        arch_cpu_caps_set(&caps->raw, ARCH_CPU_FEAT_X86_FSGSBASE);
        arch_cpu_caps_set(&caps->usable, ARCH_CPU_FEAT_X86_FSGSBASE);
    }
    if (ebx & (1 << 10)) {
        arch_cpu_caps_set(&caps->raw, ARCH_CPU_FEAT_X86_INVPCID);
        arch_cpu_caps_set(&caps->usable, ARCH_CPU_FEAT_X86_INVPCID);
    }
    if (ebx & (1 << 29)) {
        arch_cpu_caps_set(&caps->raw, ARCH_CPU_FEAT_X86_SHA);
        arch_cpu_caps_set(&caps->raw, ARCH_CPU_FEAT_COMMON_SHA);
        arch_cpu_caps_set(&caps->usable, ARCH_CPU_FEAT_X86_SHA);
        arch_cpu_caps_set(&caps->usable, ARCH_CPU_FEAT_COMMON_SHA);
    }

    // ecx bits
    if (ecx & (1 << 17)) {
        arch_cpu_caps_set(&caps->raw, ARCH_CPU_FEAT_X86_PCID);
        arch_cpu_caps_set(&caps->usable, ARCH_CPU_FEAT_X86_PCID);
    }

    // Vector Features Gating
    x86_cpuid(1, 0, &eax, &ebx, &ecx, &edx);
    if (ecx & (1 << 28)) arch_cpu_caps_set(&caps->raw, ARCH_CPU_FEAT_X86_AVX);
    if (ecx & (1 << 12)) arch_cpu_caps_set(&caps->raw, ARCH_CPU_FEAT_X86_FMA);

    x86_cpuid(7, 0, &eax, &ebx, &ecx, &edx);
    if (ebx & (1 << 5)) arch_cpu_caps_set(&caps->raw, ARCH_CPU_FEAT_X86_AVX2);

    if (osxsave) {
        uint64_t xcr0 = x86_read_xcr0();
        // check if XMM (bit 1) and YMM (bit 2) state are enabled by OS
        if ((xcr0 & 6) == 6) {
            if (arch_cpu_caps_test(&caps->raw, ARCH_CPU_FEAT_X86_AVX)) {
                arch_cpu_caps_set(&caps->usable, ARCH_CPU_FEAT_X86_AVX);
                arch_cpu_caps_set(&caps->usable, ARCH_CPU_FEAT_COMMON_VECTOR); // vector available
            }
            if (arch_cpu_caps_test(&caps->raw, ARCH_CPU_FEAT_X86_FMA)) {
                arch_cpu_caps_set(&caps->usable, ARCH_CPU_FEAT_X86_FMA);
            }
            if (arch_cpu_caps_test(&caps->raw, ARCH_CPU_FEAT_X86_AVX2)) {
                arch_cpu_caps_set(&caps->usable, ARCH_CPU_FEAT_X86_AVX2);
            }
        }
    }
}

void arch_cpu_caps_init(void) {
    arch_cpu_caps_record_t boot_caps;
    x86_probe_caps(&boot_caps);
    cpu_caps_state_set_boot(&boot_caps);
}

void arch_cpu_caps_init_ap(void) {
}
