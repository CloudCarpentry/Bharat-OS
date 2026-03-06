#include "hal/hal.h"

// x86_64 Specific HAL Implementation

void hal_cpu_halt(void) {
    // Inject x86 assembly for HLT instruction
    __asm__ volatile("hlt");
}

void hal_cpu_enable_interrupts(void) {
    __asm__ volatile("sti");
}

void hal_cpu_disable_interrupts(void) {
    __asm__ volatile("cli");
}

void hal_init(void) {
    // Setup IDT, GDT for x86_64
}

void hal_tlb_flush(unsigned long long vaddr) {
    __asm__ volatile("invlpg (%0)" :: "r"(vaddr) : "memory");
}
