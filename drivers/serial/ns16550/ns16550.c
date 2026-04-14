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

#define NS16550_LSR_THRE 0x20 /* Transmit Holding Register Empty */
#define NS16550_LCR_DLAB 0x80 /* Divisor Latch Access Bit */
#define NS16550_LCR_WLEN8 0x03 /* Word Length 8 bits */

#if defined(__x86_64__) || defined(__i386__)
static inline uint8_t x86_inb(uint16_t port) {
    uint8_t value;
    __asm__ volatile("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

static inline void x86_outb(uint16_t port, uint8_t value) {
    __asm__ volatile("outb %0, %1" : : "a"(value), "Nd"(port));
}
#endif

static inline uint8_t mmio_read8(uintptr_t addr) {
    return *(volatile uint8_t *)addr;
}

static inline void mmio_write8(uintptr_t addr, uint8_t val) {
    *(volatile uint8_t *)addr = val;
}

static uint8_t ns16550_read_reg(uart_device_t *dev, uint32_t offset) {
#if defined(__x86_64__) || defined(__i386__)
    return x86_inb(dev->base + (offset << dev->reg_shift));
#else
    return mmio_read8(dev->base + (offset << dev->reg_shift));
#endif
}

static void ns16550_write_reg(uart_device_t *dev, uint32_t offset, uint8_t val) {
#if defined(__x86_64__) || defined(__i386__)
    x86_outb(dev->base + (offset << dev->reg_shift), val);
#else
    mmio_write8(dev->base + (offset << dev->reg_shift), val);
#endif
}

static bool ns16550_init(uart_device_t *dev) {
    if (!dev || !dev->base) return false;

    // Skip configuration if input clock is 0 (assumes firmware already set it up)
    if (dev->input_clock_hz == 0 || dev->baud_rate == 0) {
        return true;
    }

    // Disable interrupts
    ns16550_write_reg(dev, NS16550_IER, 0);

    // Set baud rate (DLAB=1)
    ns16550_write_reg(dev, NS16550_LCR, NS16550_LCR_DLAB);
    uint32_t divisor = dev->input_clock_hz / (16 * dev->baud_rate);
    ns16550_write_reg(dev, NS16550_DLL, divisor & 0xFF);
    ns16550_write_reg(dev, NS16550_DLM, (divisor >> 8) & 0xFF);

    // 8 data bits, no parity, 1 stop bit (DLAB=0)
    ns16550_write_reg(dev, NS16550_LCR, NS16550_LCR_WLEN8);

    // Enable FIFO
    ns16550_write_reg(dev, NS16550_FCR, 0x01);

    return true;
}

static bool ns16550_tx_ready(uart_device_t *dev) {
    return (ns16550_read_reg(dev, NS16550_LSR) & NS16550_LSR_THRE) != 0;
}

static void ns16550_putc(uart_device_t *dev, char c) {
    volatile uint32_t timeout = 1000000;
    while (!ns16550_tx_ready(dev) && --timeout) {
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
