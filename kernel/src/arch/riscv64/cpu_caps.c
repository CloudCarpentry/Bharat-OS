#include "arch/arch_cpu_caps.h"
#include "../common/cpu_caps_state.h"
#include <stdint.h>

#define READ_CSR(reg) \
    ({ unsigned long __v; \
       __asm__ __volatile__ ("csrr %0, " #reg : "=r" (__v) : : "memory"); \
       __v; })

static void riscv64_probe_caps(arch_cpu_caps_record_t *caps) {
    arch_cpu_caps_zero(&caps->raw);
    arch_cpu_caps_zero(&caps->usable);

    unsigned long misa = READ_CSR(misa);

    // Check 'V' for Vector
    if (misa & (1UL << ('V' - 'A'))) {
        arch_cpu_caps_set(&caps->raw, ARCH_CPU_FEAT_RISCV_V);
        // We set raw 'V', but intentionally leave usable 'V' / 'COMMON_VECTOR'
        // disabled until VLEN safe context switches are active.
    }
}

void arch_cpu_caps_init(void) {
    arch_cpu_caps_record_t boot_caps;
    riscv64_probe_caps(&boot_caps);
    cpu_caps_state_set_boot(&boot_caps);
}

void arch_cpu_caps_init_ap(void) {
}
