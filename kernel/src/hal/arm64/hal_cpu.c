#include "hal/hal.h"

#include <stdint.h>

// ARM AArch64 Specific HAL Implementation

#define PL011_BASE 0x09000000UL
#define PL011_DR   0x00UL
#define PL011_FR   0x18UL
#define PL011_FR_TXFF (1U << 5)
#define PL011_FR_RXFE (1U << 4)

static inline volatile uint32_t* pl011_reg(uintptr_t offset) {
    return (volatile uint32_t*)(PL011_BASE + offset);
}

void hal_serial_init(void) {
    // QEMU virt board provides PL011 at 0x09000000 with firmware defaults.
}

void hal_serial_write_char(char c) {
    while ((*pl011_reg(PL011_FR) & PL011_FR_TXFF) != 0U) {
    }

    *pl011_reg(PL011_DR) = (uint32_t)c;
}

void hal_serial_write(const char* s) {
    if (!s) {
        return;
    }

    while (*s != '\0') {
        if (*s == '\n') {
            hal_serial_write_char('\r');
        }
        hal_serial_write_char(*s++);
    }
}

int hal_serial_read_char(void) {
    if ((*pl011_reg(PL011_FR) & PL011_FR_RXFE) != 0U) {
        return -1;
    }

    return (int)(*pl011_reg(PL011_DR) & 0xFFU);
}

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
    hal_serial_init();
}

void hal_tlb_flush(unsigned long long vaddr) {
    __asm__ volatile("tlbi vae1is, %0\n\tdsb sy\n\tisb" :: "r"(vaddr >> 12) : "memory");
}


int hal_interrupt_controller_init(void) {
    // TODO: initialize GIC distributor/redistributor.
    return 0;
}

int hal_interrupt_route(uint32_t irq, uint32_t target_core) {
    (void)irq;
    (void)target_core;
    return 0;
}

int hal_timer_source_init(uint32_t tick_hz) {
    (void)tick_hz;
    // TODO: configure generic timer CNTP/CNTV periodic tick.
    return 0;
}
