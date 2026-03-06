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

void hal_init(void) {
    // Set Vector Base Address Register (VBAR_EL1)
    // Configure MMU (TCR_EL1, MAIR_EL1)
}
