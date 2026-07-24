#include "hal/hal_boot.h"

void secondary_entry_arch_early(void) {
    // Setup EL1 state, TTBR0_EL1, TCR_EL1 for secondary core
}

void secondary_entry_arch_late(void) {
    // Enable local interrupts (PSTATE.I)
}

int hal_boot_start_cpu(uint32_t cpu_id, uint64_t entry_point) {
    // Make SMC/HVC call to PSCI CPU_ON
    return 0;
}

// Global boot info specific to ARM64
static bharat_boot_info_t g_arm64_boot_info;

bharat_boot_info_t* hal_boot_get_info(void) {
    return &g_arm64_boot_info;
}
