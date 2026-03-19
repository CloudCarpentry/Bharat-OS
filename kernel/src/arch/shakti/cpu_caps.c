#include "arch/arch_cpu_caps.h"
#include "../common/cpu_caps_state.h"

static void shakti_probe_caps(arch_cpu_caps_record_t *caps) {
    arch_cpu_caps_zero(&caps->raw);
    arch_cpu_caps_zero(&caps->usable);
}

void arch_cpu_caps_init(void) {
    arch_cpu_caps_record_t boot_caps;
    shakti_probe_caps(&boot_caps);
    cpu_caps_state_set_boot(&boot_caps);
}

void arch_cpu_caps_init_ap(void) {
}
