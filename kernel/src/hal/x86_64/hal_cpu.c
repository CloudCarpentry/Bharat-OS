#include "hal/hal.h"

#include <stdint.h>

// x86_64 Specific HAL Implementation

#define COM1_PORT 0x3F8

static inline uint8_t x86_inb(uint16_t port) {
    uint8_t value;
    __asm__ volatile("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

static inline void x86_outb(uint16_t port, uint8_t value) {
    __asm__ volatile("outb %0, %1" : : "a"(value), "Nd"(port));
}

void hal_serial_init(void) {
    // Disable interrupts
    x86_outb(COM1_PORT + 1, 0x00);
    // Enable DLAB
    x86_outb(COM1_PORT + 3, 0x80);
    // Set baud divisor to 3 (38400 baud)
    x86_outb(COM1_PORT + 0, 0x03);
    x86_outb(COM1_PORT + 1, 0x00);
    // 8 bits, no parity, one stop bit
    x86_outb(COM1_PORT + 3, 0x03);
    // Enable FIFO, clear them, with 14-byte threshold
    x86_outb(COM1_PORT + 2, 0xC7);
    // IRQs enabled, RTS/DSR set
    x86_outb(COM1_PORT + 4, 0x0B);
}

void hal_serial_write_char(char c) {
    // Wait for transmit holding register empty
    while ((x86_inb(COM1_PORT + 5) & 0x20U) == 0U) {
    }

    x86_outb(COM1_PORT, (uint8_t)c);
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
    // Data ready bit
    if ((x86_inb(COM1_PORT + 5) & 0x01U) == 0U) {
        return -1;
    }
    return (int)x86_inb(COM1_PORT);
}

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
    hal_serial_init();
}

void hal_tlb_flush(unsigned long long vaddr) {
    __asm__ volatile("invlpg (%0)" :: "r"(vaddr) : "memory");
}

void hal_send_ipi_payload(uint32_t target_core, uint64_t payload) {
    (void)target_core;
    (void)payload;
    // TODO: program local APIC/x2APIC ICR for inter-core notification.
}
