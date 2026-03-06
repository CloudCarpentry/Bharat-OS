#include "hal/hal.h"

// ARM AArch64 Specific HAL Implementation

void hal_cpu_halt(void) {
    // Wait For Interrupt instruction
    __asm__ volatile("wfi");
}

void hal_cpu_enable_interrupts(void) {
    // Enable IRQ and FIQ
    __asm__ volatile("msr daifclr, #3");
}

void hal_cpu_disable_interrupts(void) {
    // Disable IRQ and FIQ
    __asm__ volatile("msr daifset, #3");
}


void hal_send_ipi_payload(uint32_t target_core, uint64_t payload) {
    (void)target_core;
    (void)payload;
    // TODO: wire to GIC SGI path for ARM64 SMP systems.
}

void hal_init(void) {
    // Set Vector Base Address Register (VBAR_EL1)
    // Configure MMU (TCR_EL1, MAIR_EL1)
}

void hal_tlb_flush(unsigned long long vaddr) {
    __asm__ volatile("tlbi vae1is, %0\n\tdsb sy\n\tisb" :: "r"(vaddr >> 12) : "memory");
}
