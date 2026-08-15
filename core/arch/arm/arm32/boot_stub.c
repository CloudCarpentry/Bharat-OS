#include "arch/cpu_relax.h"
#include "hal/hal_boot.h"

void secondary_entry_arch_early(void) {}

void secondary_entry_arch_late(void) {}

int hal_boot_start_cpu(uint32_t cpu_id, uint64_t entry_point) {
    (void)cpu_id;
    (void)entry_point;
    return -1;
}

static bharat_boot_info_t g_arm32_boot_info;

bharat_boot_info_t *hal_boot_get_info(void) {
    return &g_arm32_boot_info;
}

void _secondary_trampoline(void) {
    for (;;) {
        arch_cpu_relax();
    }
}
