#include "arch/arch_cpu_caps.h"
#include "../../common/cpu_caps_state.h"
#include <stdint.h>

extern uint32_t hal_cpu_get_id(void);

static void riscv32_probe_caps(arch_cpu_caps_record_t *caps) {
    arch_cpu_caps_zero(&caps->raw);
    arch_cpu_caps_zero(&caps->usable);

    /* The generic RV32 target is built for the A extension baseline. */
    arch_cpu_caps_set(&caps->raw, ARCH_CPU_FEAT_COMMON_STRONG_ATOMICS);
    arch_cpu_caps_set(&caps->usable, ARCH_CPU_FEAT_COMMON_STRONG_ATOMICS);
}

void arch_cpu_caps_init(void) {
    arch_cpu_caps_record_t boot_caps;
    riscv32_probe_caps(&boot_caps);
    cpu_caps_state_set_boot(&boot_caps);
}

void arch_cpu_caps_init_ap(void) {
    arch_cpu_caps_record_t ap_caps;
    riscv32_probe_caps(&ap_caps);
    cpu_caps_state_set_ap(hal_cpu_get_id(), &ap_caps);
}

void arch_cpu_caps_export_hal_features(const arch_cpu_caps_record_t *arch, void *out_ptr) {
    (void)arch;
    (void)out_ptr;
}
