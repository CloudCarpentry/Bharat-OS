#include "hal/hal_boot.h"

void secondary_entry_arch_early(void) {
    // Setup initial CSRs (satp, stvec, sscratch) for secondary hart
}

void secondary_entry_arch_late(void) {
    // Enable interrupts (sstatus.SIE)
}

int hal_boot_start_cpu(uint32_t cpu_id, uint64_t entry_point) {
    (void)cpu_id;
    (void)entry_point;
    /* SBI HSM start is not wired yet; fail closed instead of falsely
     * claiming that the hart was launched. */
    return -1;
}

// Global boot info specific to RISC-V
static bharat_boot_info_t g_riscv64_boot_info;

bharat_boot_info_t* hal_boot_get_info(void) {
    return &g_riscv64_boot_info;
}
