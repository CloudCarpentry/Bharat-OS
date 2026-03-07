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

// --- Trap / Interrupt Handling ---

extern void vector_table_el1(void); // Defined in assembly, or we define a simple dummy one
static uint8_t dummy_vector_table[2048] __attribute__((aligned(2048)));

void hal_init(void) {
    // Set Vector Base Address Register (VBAR_EL1)
    // For now, point to a dummy aligned table if we don't have the real assembly one linked
    __asm__ volatile("msr vbar_el1, %0" : : "r"((uint64_t)&dummy_vector_table));

    // Configure MMU (TCR_EL1, MAIR_EL1)
    hal_serial_init();
}

void hal_tlb_flush(unsigned long long vaddr) {
    __asm__ volatile("tlbi vae1is, %0\n\tdsb sy\n\tisb" :: "r"(vaddr >> 12) : "memory");
}


// --- GICv2 / GICv3 Definitions (QEMU virt usually uses GICv2/v3, we'll assume a basic GICv2 for simplicity here or a GICv3 in legacy mode) ---
// Note: Real implementations detect GIC version from device tree, but QEMU virt often defaults to GICv2 unless specified.
#define GICD_BASE 0x08000000UL // Distributor
#define GICC_BASE 0x08010000UL // CPU Interface

#define GICD_CTLR (GICD_BASE + 0x000)
#define GICD_ISENABLER(n) (GICD_BASE + 0x100 + ((n) * 4))
#define GICD_IPRIORITYR(n) (GICD_BASE + 0x400 + ((n) * 4))
#define GICD_ITARGETSR(n) (GICD_BASE + 0x800 + ((n) * 4))

#define GICC_CTLR (GICC_BASE + 0x000)
#define GICC_PMR  (GICC_BASE + 0x004)


int hal_interrupt_controller_init(void) {
    // Initialize GIC Distributor
    volatile uint32_t* gicd_ctlr = (volatile uint32_t*)GICD_CTLR;
    *gicd_ctlr = 0; // Disable distributor

    // In a real system, we'd configure all interrupts to a default priority and route them to CPU 0
    // For now, just enable the distributor
    *gicd_ctlr = 1; // Enable Group 0

    // Initialize GIC CPU Interface
    volatile uint32_t* gicc_pmr = (volatile uint32_t*)GICC_PMR;
    *gicc_pmr = 0xF0; // Accept all interrupts (priority mask)

    volatile uint32_t* gicc_ctlr = (volatile uint32_t*)GICC_CTLR;
    *gicc_ctlr = 1; // Enable CPU interface

    return 0;
}

int hal_interrupt_route(uint32_t irq, uint32_t target_core) {
    if (irq >= 1020) return -1; // Max valid INTID in GICv2

    // Set priority to 0xA0
    volatile uint8_t* gicd_ipriorityr = (volatile uint8_t*)GICD_IPRIORITYR(irq / 4);
    gicd_ipriorityr[irq % 4] = 0xA0;

    // Set target core (for SPIs, IRQ 32+)
    if (irq >= 32) {
        volatile uint8_t* gicd_itargetsr = (volatile uint8_t*)GICD_ITARGETSR(irq / 4);
        gicd_itargetsr[irq % 4] = (1 << target_core);
    }

    // Enable interrupt
    volatile uint32_t* gicd_isenabler = (volatile uint32_t*)GICD_ISENABLER(irq / 32);
    *gicd_isenabler = (1 << (irq % 32));

    return 0;
}

int hal_timer_source_init(uint32_t tick_hz) {
    if (tick_hz == 0U) return -1;

    uint64_t freq;
    __asm__ volatile("mrs %0, cntfrq_el0" : "=r"(freq));

    if (freq == 0) return -1;

    uint64_t ticks_per_period = freq / tick_hz;

    // Set timer value
    __asm__ volatile("msr cntp_tval_el0, %0" : : "r"(ticks_per_period));

    // Enable timer and unmask interrupt
    // cntp_ctl_el0: bit 0 = ENABLE, bit 1 = IMASK
    uint64_t ctl = 1;
    __asm__ volatile("msr cntp_ctl_el0, %0" : : "r"(ctl));

    // The generic timer usually routes to a specific PPI, e.g., 30 for Physical Timer
    hal_interrupt_route(30, 0);

    return 0;
}
