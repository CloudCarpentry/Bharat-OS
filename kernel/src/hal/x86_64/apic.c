#include "hal/hal_irq.h"
#include "hal/hal_boot.h"

#define LAPIC_SVR_OFFSET 0x0F0
#define LAPIC_ICR_LOW_OFFSET 0x300
#define LAPIC_ICR_HIGH_OFFSET 0x310
#define LAPIC_EOI_OFFSET 0x0B0

// We assume these are populated during ACPI/MADT parsing
uint32_t* g_lapic_base = (uint32_t*)0xFEE00000;

static inline void lapic_write(uint32_t offset, uint32_t value) {
    *(volatile uint32_t*)((uint64_t)g_lapic_base + offset) = value;
}

static inline uint32_t lapic_read(uint32_t offset) {
    return *(volatile uint32_t*)((uint64_t)g_lapic_base + offset);
}

int hal_irq_init_boot(void) {
    // Disable legacy PIC via IO ports (minimal legacy PC compatible method)
    // outb(0xA1, 0xFF); outb(0x21, 0xFF);
    return 0;
}

int hal_irq_init_cpu_local(void) {
    // Enable LAPIC by setting spurious interrupt vector and bit 8 (APIC Software Enable)
    lapic_write(LAPIC_SVR_OFFSET, 0x1FF | 0x100);
    return 0;
}

int hal_irq_enable(uint32_t vector) {
    // Minimal IOAPIC RTE configuration goes here in real code
    return 0;
}

int hal_irq_disable(uint32_t vector) {
    // Minimal IOAPIC RTE configuration goes here in real code
    return 0;
}

int hal_ipi_send(uint32_t cpu_id, uint32_t reason_vector) {
    // Send IPI via ICR (Interrupt Command Register)
    lapic_write(LAPIC_ICR_HIGH_OFFSET, (cpu_id << 24));
    lapic_write(LAPIC_ICR_LOW_OFFSET, reason_vector | 0x00004000); // Fixed delivery, no shorthand
    return 0;
}

void hal_irq_eoi(uint32_t vector) {
    // Write 0 to EOI register
    lapic_write(LAPIC_EOI_OFFSET, 0);
}
