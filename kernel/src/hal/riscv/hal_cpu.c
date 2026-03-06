#include "hal/hal.h"

#include "../../boot/riscv/sbi.h"

// RISC-V Specific HAL Implementation (RV64 / Shakti)

void hal_serial_init(void) {
    // OpenSBI console is already available; no additional UART init required.
}

void hal_serial_write_char(char c) {
    sbi_console_putchar((int)c);
}

void hal_serial_write(const char* s) {
    if (!s) {
        return;
    }

    while (*s != '\0') {
        if (*s == '\n') {
            sbi_console_putchar('\r');
        }
        sbi_console_putchar(*s++);
    }
}

int hal_serial_read_char(void) {
    return sbi_console_getchar();
}

void hal_riscv_send_ipi_payload(const unsigned long* hart_mask, uint64_t payload) {
    sbi_send_ipi_payload(hart_mask, payload);
}

void hal_send_ipi_payload(uint32_t target_core, uint64_t payload) {
    unsigned long hart_mask = (1UL << target_core);
    hal_riscv_send_ipi_payload(&hart_mask, payload);
}

void hal_cpu_halt(void) {
    // Wait for interrupt instruction
    __asm__ volatile("wfi");
}

void hal_cpu_enable_interrupts(void) {
#ifdef CONFIG_RISCV_M_MODE
    // Set MIE (Machine Interrupt Enable) bit in mstatus CSR
    __asm__ volatile("csrsi mstatus, 8");
#else
    // Set SIE (Supervisor Interrupt Enable) bit in sstatus CSR
    __asm__ volatile("csrsi sstatus, 2");
#endif
}

void hal_cpu_disable_interrupts(void) {
#ifdef CONFIG_RISCV_M_MODE
    // Clear MIE (Machine Interrupt Enable) bit in mstatus CSR
    __asm__ volatile("csrci mstatus, 8");
#else
    // Clear SIE (Supervisor Interrupt Enable) bit in sstatus CSR
    __asm__ volatile("csrci sstatus, 2");
#endif
}

void hal_init(void) {
    // Setup trap vectors (stvec) for Supervisor mode
    // Setup SBI console if running in Supervisor mode, or physical UART if Machine mode
    hal_serial_init();
}

void hal_tlb_flush(unsigned long long vaddr) {
    __asm__ volatile("sfence.vma %0, x0" :: "r"(vaddr) : "memory");
}
