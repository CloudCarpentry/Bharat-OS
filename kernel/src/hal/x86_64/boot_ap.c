#include "hal/hal_boot.h"

void secondary_entry_arch_early(void) {
    // Load GDT/IDT/CR3 for secondary core
}

void secondary_entry_arch_late(void) {
    // Enable interrupts
}

int hal_boot_start_cpu(uint32_t cpu_id, uint64_t entry_point) {
    // Send INIT IPI
    // Send STARTUP IPI
    return 0;
}

// Global boot info specific to x86_64
static bharat_boot_info_t g_x86_64_boot_info;

bharat_boot_info_t* hal_boot_get_info(void) {
    return &g_x86_64_boot_info;
}
