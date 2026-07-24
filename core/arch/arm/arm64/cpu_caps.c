#include "arch/arch_cpu_caps.h"
#include "../../common/cpu_caps_state.h"
#include <stdint.h>

extern uint32_t hal_cpu_get_id(void);

#define READ_SYSREG(reg) \
    ({ uint64_t _val; __asm__ volatile("mrs %0, " #reg : "=r"(_val)); _val; })

static void arm64_probe_caps(arch_cpu_caps_record_t *caps) {
    arch_cpu_caps_zero(&caps->raw);
    arch_cpu_caps_zero(&caps->usable);

    uint64_t isar0 = READ_SYSREG(id_aa64isar0_el1);
    uint64_t pfr0  = READ_SYSREG(id_aa64pfr0_el1);

    // ID_AA64ISAR0_EL1
    uint64_t aes = (isar0 >> 4) & 0xf;
    if (aes >= 1) {
        arch_cpu_caps_set(&caps->raw, ARCH_CPU_FEAT_ARM64_AES);
        arch_cpu_caps_set(&caps->raw, ARCH_CPU_FEAT_COMMON_AES);
        arch_cpu_caps_set(&caps->usable, ARCH_CPU_FEAT_ARM64_AES);
        arch_cpu_caps_set(&caps->usable, ARCH_CPU_FEAT_COMMON_AES);
    }
    if (aes >= 2) {
        arch_cpu_caps_set(&caps->raw, ARCH_CPU_FEAT_ARM64_PMULL);
        arch_cpu_caps_set(&caps->raw, ARCH_CPU_FEAT_COMMON_PMULL);
        arch_cpu_caps_set(&caps->usable, ARCH_CPU_FEAT_ARM64_PMULL);
        arch_cpu_caps_set(&caps->usable, ARCH_CPU_FEAT_COMMON_PMULL);
    }

    uint64_t sha1 = (isar0 >> 8) & 0xf;
    uint64_t sha2 = (isar0 >> 12) & 0xf;
    if (sha1 >= 1 || sha2 >= 1) {
        arch_cpu_caps_set(&caps->raw, ARCH_CPU_FEAT_ARM64_SHA1);
        arch_cpu_caps_set(&caps->raw, ARCH_CPU_FEAT_COMMON_SHA);
        arch_cpu_caps_set(&caps->usable, ARCH_CPU_FEAT_ARM64_SHA1);
        arch_cpu_caps_set(&caps->usable, ARCH_CPU_FEAT_COMMON_SHA);
    }

    uint64_t lse = (isar0 >> 20) & 0xf;
    if (lse >= 1) {
        arch_cpu_caps_set(&caps->raw, ARCH_CPU_FEAT_ARM64_LSE);
        arch_cpu_caps_set(&caps->raw, ARCH_CPU_FEAT_COMMON_STRONG_ATOMICS);
        arch_cpu_caps_set(&caps->usable, ARCH_CPU_FEAT_ARM64_LSE);
        arch_cpu_caps_set(&caps->usable, ARCH_CPU_FEAT_COMMON_STRONG_ATOMICS);
    }

    // ID_AA64PFR0_EL1
    uint64_t asimd = (pfr0 >> 20) & 0xf;
    if (asimd == 0 || asimd == 1) { // 0 or 1 means supported, 0xf means not implemented
        arch_cpu_caps_set(&caps->raw, ARCH_CPU_FEAT_ARM64_ASIMD);
        arch_cpu_caps_set(&caps->usable, ARCH_CPU_FEAT_ARM64_ASIMD);
        arch_cpu_caps_set(&caps->usable, ARCH_CPU_FEAT_COMMON_VECTOR); // vector enabled via ASIMD
    }

    uint64_t sve = (pfr0 >> 32) & 0xf;
    if (sve >= 1) {
        arch_cpu_caps_set(&caps->raw, ARCH_CPU_FEAT_ARM64_SVE);

        // SVE is exposed raw, but intentionally NOT marked usable yet
        // until kernel context switch handles variable SVE states reliably.

        uint64_t zfr0 = READ_SYSREG(S3_0_C0_C4_4);
        uint64_t sve2 = (zfr0 >> 0) & 0xf;
        if (sve2 >= 1) {
            arch_cpu_caps_set(&caps->raw, ARCH_CPU_FEAT_ARM64_SVE2);
        }
    }
}

void arch_cpu_caps_init(void) {
    arch_cpu_caps_record_t boot_caps;
    arm64_probe_caps(&boot_caps);
    cpu_caps_state_set_boot(&boot_caps);
}

void arch_cpu_caps_init_ap(void) {
    arch_cpu_caps_record_t ap_caps;
    arm64_probe_caps(&ap_caps);
    cpu_caps_state_set_ap(hal_cpu_get_id(), &ap_caps);
}
