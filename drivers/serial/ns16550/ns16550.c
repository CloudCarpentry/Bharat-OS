#include "drivers/serial/uart_driver.h"
#include <stddef.h>

/* NS16550 Register Offsets */
#define NS16550_THR 0x0 /* Transmit Holding Register */
#define NS16550_RBR 0x0 /* Receive Buffer Register */
#define NS16550_DLL 0x0 /* Divisor Latch LSB */
#define NS16550_DLM 0x1 /* Divisor Latch MSB */
#define NS16550_IER 0x1 /* Interrupt Enable Register */
#define NS16550_IIR 0x2 /* Interrupt Identity Register */
#define NS16550_FCR 0x2 /* FIFO Control Register */
#define NS16550_LCR 0x3 /* Line Control Register */
#define NS16550_MCR 0x4 /* Modem Control Register */
#define NS16550_LSR 0x5 /* Line Status Register */

#define LSR_THRE 0x20 /* Transmit Holding Register Empty */

static inline uint8_t x86_inb(uint16_t port) {
    uint8_t value;
#if defined(__x86_64__)
    __asm__ volatile("inb %1, %0" : "=a"(value) : "Nd"(port));
#endif
    return value;
}

static inline void x86_outb(uint16_t port, uint8_t value) {
#if defined(__x86_64__)
    __asm__ volatile("outb %0, %1" : : "a"(value), "Nd"(port));
#endif
}

static inline uint8_t mmio_read8(uintptr_t addr) {
    return *(volatile uint8_t *)addr;
}

static inline void mmio_write8(uintptr_t addr, uint8_t val) {
    *(volatile uint8_t *)addr = val;
}

static uint8_t ns16550_read_reg(uart_device_t *dev, uint32_t offset) {
#if defined(__x86_64__)
    return x86_inb(dev->base + (offset << dev->reg_shift));
#else
    return mmio_read8(dev->base + (offset << dev->reg_shift));
#endif
}

static void ns16550_write_reg(uart_device_t *dev, uint32_t offset, uint8_t val) {
#if defined(__x86_64__)
    x86_outb(dev->base + (offset << dev->reg_shift), val);
#else
    mmio_write8(dev->base + (offset << dev->reg_shift), val);
#endif
}

static bool ns16550_init(uart_device_t *dev) {
    if (!dev || !dev->base) return false;

    // Basic polled mode init: Disable interrupts
    ns16550_write_reg(dev, NS16550_IER, 0x00);

    // Enable FIFOs, clear them
    ns16550_write_reg(dev, NS16550_FCR, 0x07);

    // 8 data bits, 1 stop bit, no parity
    ns16550_write_reg(dev, NS16550_LCR, 0x03);

    // Set DTR, RTS
    ns16550_write_reg(dev, NS16550_MCR, 0x03);

    return true;
}

static bool ns16550_tx_ready(uart_device_t *dev) {
    return (ns16550_read_reg(dev, NS16550_LSR) & LSR_THRE) != 0;
}

static void ns16550_putc(uart_device_t *dev, char c) {
    while (!ns16550_tx_ready(dev)) {
        // Wait
    }
    ns16550_write_reg(dev, NS16550_THR, (uint8_t)c);
}

static size_t ns16550_write(uart_device_t *dev, const char *data, size_t len) {
    for (size_t i = 0; i < len; i++) {
        if (data[i] == '\n') {
            ns16550_putc(dev, '\r');
        }
        ns16550_putc(dev, data[i]);
    }
    return len;
}

static void ns16550_flush(uart_device_t *dev) {
    while (!ns16550_tx_ready(dev)) {
        // Wait for clear
    }
}

static console_caps_t ns16550_query_caps(uart_device_t *dev) {
    (void)dev;
    return CON_CAP_WRITE_POLL | CON_CAP_PANIC_SAFE;
}

const uart_driver_ops_t uart_ns16550_ops = {
    .init = ns16550_init,
    .tx_ready = ns16550_tx_ready,
    .putc = ns16550_putc,
    .write = ns16550_write,
    .flush = ns16550_flush,
    .query_caps = ns16550_query_caps
};
