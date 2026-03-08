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

void hal_cpu_reboot(void) {
    // Attempt system reset using PSCI
    __asm__ volatile(
        "ldr x0, =0x84000009\n" // PSCI_SYSTEM_RESET
        "hvc #0\n"
    );
    while (1) {
        __asm__ volatile("wfi");
    }
}

static void print_hex(uint64_t val) {
    char buf[17];
    buf[16] = '\0';
    for (int i = 15; i >= 0; i--) {
        uint8_t nibble = val & 0xF;
        buf[i] = nibble < 10 ? '0' + nibble : 'a' + (nibble - 10);
        val >>= 4;
    }
    hal_serial_write("0x");
    hal_serial_write(buf);
}

void hal_cpu_dump_state(void) {
    uint64_t far_el1, esr_el1, elr_el1, spsr_el1, x29, sp;
    __asm__ volatile("mrs %0, far_el1" : "=r"(far_el1));
    __asm__ volatile("mrs %0, esr_el1" : "=r"(esr_el1));
    __asm__ volatile("mrs %0, elr_el1" : "=r"(elr_el1));
    __asm__ volatile("mrs %0, spsr_el1" : "=r"(spsr_el1));
    __asm__ volatile("mov %0, x29" : "=r"(x29));
    __asm__ volatile("mov %0, sp" : "=r"(sp));

    hal_serial_write("\n--- ARM64 CPU State Dump ---\n");
    hal_serial_write("FAR_EL1: "); print_hex(far_el1); hal_serial_write("\n");
    hal_serial_write("ESR_EL1: "); print_hex(esr_el1); hal_serial_write("\n");
    hal_serial_write("ELR_EL1: "); print_hex(elr_el1); hal_serial_write("\n");
    hal_serial_write("SPSR_EL1: "); print_hex(spsr_el1); hal_serial_write("\n");
    hal_serial_write("FP (x29): "); print_hex(x29); hal_serial_write("\n");
    hal_serial_write("SP: "); print_hex(sp); hal_serial_write("\n");

    hal_serial_write("\nStack Trace (Frame Pointers):\n");
    uint64_t current_fp = x29;
    int depth = 0;
    while (current_fp != 0 && current_fp >= 0x1000 && depth < 10) {
        uint64_t* frame = (uint64_t*)current_fp;
        uint64_t next_fp = frame[0];
        uint64_t ret_addr = frame[1];

        hal_serial_write("  [");
        char depth_str[2] = {(char)('0' + depth), '\0'};
        hal_serial_write(depth_str);
        hal_serial_write("] pc="); print_hex(ret_addr);
        hal_serial_write(" fp="); print_hex(next_fp); hal_serial_write("\n");

        if (next_fp <= current_fp) {
            break; // Stop if frame pointer is not strictly increasing
        }
        current_fp = next_fp;
        depth++;
    }
    hal_serial_write("-----------------------------\n");
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

extern void vector_table_el1(void); // Defined in trap_entry.S

void hal_init(void) {
    // Set Vector Base Address Register (VBAR_EL1)
    __asm__ volatile("msr vbar_el1, %0" : : "r"((uint64_t)&vector_table_el1));

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

static uint64_t g_timer_interval;

int hal_timer_source_init(uint32_t tick_hz) {
    if (tick_hz == 0U) return -1;

    uint64_t freq;
    __asm__ volatile("mrs %0, cntfrq_el0" : "=r"(freq));

    if (freq == 0) return -1;

    g_timer_interval = freq / tick_hz;

    // Set timer value
    __asm__ volatile("msr cntp_tval_el0, %0" : : "r"(g_timer_interval));

    // Enable timer and unmask interrupt
    // cntp_ctl_el0: bit 0 = ENABLE, bit 1 = IMASK (0 means unmasked)
    uint64_t ctl = 1;
    __asm__ volatile("msr cntp_ctl_el0, %0" : : "r"(ctl));

    // The generic timer usually routes to a specific PPI, e.g., 30 for Physical Timer
    hal_interrupt_route(30, 0);

    return 0;
}

void hal_timer_isr(void) {
    // Acknowledge and re-arm timer (cntp_tval_el0 is a countdown timer, writing to it re-arms it and clears the interrupt condition)
    __asm__ volatile("msr cntp_tval_el0, %0" : : "r"(g_timer_interval));

    hal_timer_tick();
}

uint32_t hal_cpu_get_id(void) {
    uint64_t mpidr;
    __asm__ volatile("mrs %0, mpidr_el1" : "=r"(mpidr));
    return (uint32_t)(mpidr & 0xFF);
}
