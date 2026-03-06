#include "hal/hal.h"

// RISC-V Specific HAL Implementation (RV64 / Shakti)

void hal_cpu_halt(void) {
    // Wait for interrupt instruction
    __asm__ volatile("wfi");
}

void hal_cpu_enable_interrupts(void) {
    // Set SIE (Supervisor Interrupt Enable) bit in sstatus CSR
    __asm__ volatile("csrsi sstatus, 2");
}

void hal_cpu_disable_interrupts(void) {
    // Clear SIE (Supervisor Interrupt Enable) bit in sstatus CSR
    __asm__ volatile("csrci sstatus, 2");
}

void hal_init(void) {
    // Setup trap vectors (mtvec)
    // Setup SBI console if running in Supervisor mode, or physical UART if Machine mode
}

void hal_tlb_flush(unsigned long long vaddr) {
    __asm__ volatile("sfence.vma %0, x0" :: "r"(vaddr) : "memory");
}
