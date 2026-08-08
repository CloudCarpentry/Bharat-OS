#include "arch/arch_cpu_caps.h"
#include "../../common/cpu_caps_state.h"
#include <stdint.h>

void arch_cpu_caps_init(void) {
    // riscv32 cpu caps is currently a stub
}

void arch_cpu_caps_init_ap(void) {
    // riscv32 cpu caps is currently a stub
}

void arch_cpu_caps_export_hal_features(const arch_cpu_caps_record_t *arch, void *out_ptr) {
    (void)arch;
    (void)out_ptr;
}
