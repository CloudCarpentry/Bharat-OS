#include "sifive_uart.h"
#include <stddef.h>

static bool sifive_uart_init(uart_device_t *dev) {
    if (!dev || !dev->base) return false;
    // Skeleton: initialize SiFive UART
    return true;
}

static bool sifive_uart_tx_ready(uart_device_t *dev) {
    (void)dev;
    // Skeleton: check TX readiness
    return true;
}

static void sifive_uart_putc(uart_device_t *dev, char c) {
    (void)dev;
    (void)c;
    // Skeleton: transmit char
}

static size_t sifive_uart_write(uart_device_t *dev, const char *data, size_t len) {
    (void)dev;
    (void)data;
    // Skeleton: transmit sequence
    return len;
}

static void sifive_uart_flush(uart_device_t *dev) {
    (void)dev;
    // Skeleton: flush FIFO
}

static console_caps_t sifive_uart_query_caps(uart_device_t *dev) {
    (void)dev;
    return CON_CAP_WRITE_POLL | CON_CAP_PANIC_SAFE;
}

const uart_driver_ops_t uart_sifive_ops = {
    .init = sifive_uart_init,
    .tx_ready = sifive_uart_tx_ready,
    .putc = sifive_uart_putc,
    .write = sifive_uart_write,
    .flush = sifive_uart_flush,
    .query_caps = sifive_uart_query_caps
};
