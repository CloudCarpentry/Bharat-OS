#include "drivers/serial/uart_driver.h"
#include <stddef.h>

/* PL011 Register Offsets */
#define PL011_DR   0x00
#define PL011_FR   0x18
#define PL011_IBRD 0x24
#define PL011_FBRD 0x28
#define PL011_LCRH 0x2c
#define PL011_CR   0x30
#define PL011_IMSC 0x38

#define PL011_FR_TXFF  (1 << 5)
#define PL011_LCRH_WLEN_8 (3 << 5)
#define PL011_LCRH_FEN (1 << 4)
#define PL011_CR_UARTEN (1 << 0)
#define PL011_CR_TXE   (1 << 8)
#define PL011_CR_RXE   (1 << 9)

static inline uint32_t mmio_read32(uintptr_t addr) {
    return *(volatile uint32_t *)addr;
}

static inline void mmio_write32(uintptr_t addr, uint32_t val) {
    *(volatile uint32_t *)addr = val;
}

static bool pl011_init(uart_device_t *dev) {
    if (!dev || !dev->base) return false;

    // Mask interrupts
    mmio_write32(dev->base + PL011_IMSC, 0);

    // Disable UART
    mmio_write32(dev->base + PL011_CR, 0);

    // 8 data bits, 1 stop bit, FIFOs enabled
    mmio_write32(dev->base + PL011_LCRH, PL011_LCRH_WLEN_8 | PL011_LCRH_FEN);

    // Enable UART, TX, RX
    mmio_write32(dev->base + PL011_CR, PL011_CR_UARTEN | PL011_CR_TXE | PL011_CR_RXE);

    return true;
}

static bool pl011_tx_ready(uart_device_t *dev) {
    return (mmio_read32(dev->base + PL011_FR) & PL011_FR_TXFF) == 0;
}

static void pl011_putc(uart_device_t *dev, char c) {
    volatile uint32_t timeout = 1000000;
    while (!pl011_tx_ready(dev) && --timeout) {
        // Wait
    }
    mmio_write32(dev->base + PL011_DR, (uint32_t)c);
}

static size_t pl011_write(uart_device_t *dev, const char *data, size_t len) {
    for (size_t i = 0; i < len; i++) {
        if (data[i] == '\n') {
            pl011_putc(dev, '\r');
        }
        pl011_putc(dev, data[i]);
    }
    return len;
}

static void pl011_flush(uart_device_t *dev) {
    while (!pl011_tx_ready(dev)) {
        // Wait
    }
}

static console_caps_t pl011_query_caps(uart_device_t *dev) {
    (void)dev;
    return CON_CAP_WRITE_POLL | CON_CAP_PANIC_SAFE;
}

const uart_driver_ops_t uart_pl011_ops = {
    .init = pl011_init,
    .tx_ready = pl011_tx_ready,
    .putc = pl011_putc,
    .write = pl011_write,
    .flush = pl011_flush,
    .query_caps = pl011_query_caps
};
