#include "hal/hal_irq.h"
#include "hal/hal_ipi.h"
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

static inline void x86_outb(uint16_t port, uint8_t value) {
  __asm__ volatile("outb %0, %1" : : "a"(value), "Nd"(port));
}

void hal_irq_init_boot(void) {
    // Enable local APIC (set Spurious Interrupt Vector Register)
    lapic_write(LAPIC_SVR_OFFSET, 0x1FF | 0x100);

    // Disable 8259 PICs to ensure only APIC is used
    x86_outb(0xA1, 0xFF);
    x86_outb(0x21, 0xFF);
}

void hal_irq_init_cpu_local(uint32_t cpu_id) {
    (void)cpu_id;
    // Enable LAPIC by setting spurious interrupt vector and bit 8 (APIC Software Enable)
    lapic_write(LAPIC_SVR_OFFSET, 0x1FF | 0x100);
}

int hal_irq_enable(uint32_t vector) {
    (void)vector;
    // IOAPIC RTE configuration
    return 0;
}

int hal_irq_disable(uint32_t vector) {
    (void)vector;
    // IOAPIC RTE configuration
    return 0;
}

uint32_t hal_irq_claim(void) {
    // x86_64 uses IDT vectors; ack is usually handled by the ISR.
    return 0;
}

void hal_irq_eoi(uint32_t irq) {
    (void)irq;
    // Write 0 to EOI register
    lapic_write(LAPIC_EOI_OFFSET, 0);
}

void hal_ipi_init_cpu_local(uint32_t cpu_id) {
    (void)cpu_id;
    // Local APIC covers IPI init
}

void hal_ipi_send(uint32_t target_cpu, hal_ipi_reason_t reason) {
    // Send IPI via ICR (Interrupt Command Register)
    // Map reason to a vector (e.g., 200 + reason)
    uint32_t reason_vector = 200 + (uint32_t)reason;
    lapic_write(LAPIC_ICR_HIGH_OFFSET, (target_cpu << 24));
    lapic_write(LAPIC_ICR_LOW_OFFSET, reason_vector | 0x00004000); // Fixed delivery, assert, edge trigger
}

void hal_ipi_broadcast(uint64_t mask, hal_ipi_reason_t reason) {
    // Use ICR shorthand for broadcast, or iterate mask
    // For simplicity, iterating mask:
    for (uint32_t i = 0; i < 64; i++) {
        if ((mask & (1ULL << i)) != 0) {
            hal_ipi_send(i, reason);
        }
    }
}
