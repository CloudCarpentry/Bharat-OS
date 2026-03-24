#include "arch/arch_cpu_caps.h"
#include "../../common/cpu_caps_state.h"
#include <stdint.h>

#define READ_CSR(reg) \
    ({ unsigned long __v; \
       __asm__ __volatile__ ("csrr %0, " #reg : "=r" (__v) : : "memory"); \
       __v; })

static void riscv64_probe_caps(arch_cpu_caps_record_t *caps) {
    arch_cpu_caps_zero(&caps->raw);
    arch_cpu_caps_zero(&caps->usable);

    // MISA is a Machine-mode CSR and cannot be read in Supervisor-mode.
    // We assume standard IMAFD features are present on generic riscv64 platforms.
    arch_cpu_caps_set(&caps->raw, ARCH_CPU_FEAT_COMMON_STRONG_ATOMICS);
    arch_cpu_caps_set(&caps->usable, ARCH_CPU_FEAT_COMMON_STRONG_ATOMICS);

    // Vector and other extensions should be detected via Device Tree or SBI.
    // For now, we rely on compile-time defines below.

#ifdef BHARAT_ISA_FEATURE_ZBA
    arch_cpu_caps_set(&caps->raw, ARCH_CPU_FEAT_RISCV_ZBA);
    arch_cpu_caps_set(&caps->usable, ARCH_CPU_FEAT_RISCV_ZBA);
#endif
#ifdef BHARAT_ISA_FEATURE_ZBB
    arch_cpu_caps_set(&caps->raw, ARCH_CPU_FEAT_RISCV_ZBB);
    arch_cpu_caps_set(&caps->usable, ARCH_CPU_FEAT_RISCV_ZBB);
#endif
#ifdef BHARAT_ISA_FEATURE_ZBC
    arch_cpu_caps_set(&caps->raw, ARCH_CPU_FEAT_RISCV_ZBC);
    arch_cpu_caps_set(&caps->usable, ARCH_CPU_FEAT_RISCV_ZBC);
#endif
#ifdef BHARAT_ISA_FEATURE_ZBS
    arch_cpu_caps_set(&caps->raw, ARCH_CPU_FEAT_RISCV_ZBS);
    arch_cpu_caps_set(&caps->usable, ARCH_CPU_FEAT_RISCV_ZBS);
#endif
}

void arch_cpu_caps_init(void) {
    arch_cpu_caps_record_t boot_caps;
    riscv64_probe_caps(&boot_caps);
    cpu_caps_state_set_boot(&boot_caps);
}

void arch_cpu_caps_init_ap(void) {
}
