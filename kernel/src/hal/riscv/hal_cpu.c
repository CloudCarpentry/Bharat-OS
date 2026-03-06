#include "hal/hal.h"

// RISC-V Specific HAL Implementation (RV64 / Shakti)

void hal_cpu_halt(void) {
    // Wait for interrupt instruction
    __asm__ volatile("wfi");
}

void hal_cpu_enable_interrupts(void) {
    // Set MIE (Machine Interrupt Enable) bit in mstatus CSR
    __asm__ volatile("csrsi mstatus, 8");
}

void hal_cpu_disable_interrupts(void) {
    // Clear MIE (Machine Interrupt Enable) bit in mstatus CSR
    __asm__ volatile("csrci mstatus, 8");
}

void hal_init(void) {
    // Setup trap vectors (mtvec)
    // Setup SBI console if running in Supervisor mode, or physical UART if Machine mode
}
