#include "drivers/serial/uart_driver.h"
#include <stddef.h>

/* Simple MMIO UART (e.g., HTIF or SBI generic fallback) */

static inline void mmio_write8(uintptr_t addr, uint8_t val) {
    *(volatile uint8_t *)addr = val;
}

static inline void mmio_write32(uintptr_t addr, uint32_t val) {
    *(volatile uint32_t *)addr = val;
}

static bool simple_init(uart_device_t *dev) {
    if (!dev || !dev->base) return false;
    return true;
}

static bool simple_tx_ready(uart_device_t *dev) {
    (void)dev;
    return true; // Assume always ready
}

static void simple_putc(uart_device_t *dev, char c) {
    if (dev->reg_width == 1) {
        mmio_write8(dev->base, (uint8_t)c);
    } else {
        mmio_write32(dev->base, (uint32_t)c);
    }
}

static size_t simple_write(uart_device_t *dev, const char *data, size_t len) {
    for (size_t i = 0; i < len; i++) {
        if (data[i] == '\n') {
            simple_putc(dev, '\r');
        }
        simple_putc(dev, data[i]);
    }
    return len;
}

static void simple_flush(uart_device_t *dev) {
    (void)dev;
    // No-op
}

static console_caps_t simple_query_caps(uart_device_t *dev) {
    (void)dev;
    return CON_CAP_WRITE_POLL | CON_CAP_PANIC_SAFE;
}

const uart_driver_ops_t uart_simple_mmio_ops = {
    .init = simple_init,
    .tx_ready = simple_tx_ready,
    .putc = simple_putc,
    .write = simple_write,
    .flush = simple_flush,
    .query_caps = simple_query_caps
};
