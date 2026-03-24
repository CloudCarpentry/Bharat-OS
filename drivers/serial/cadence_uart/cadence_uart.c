#include "cadence_uart.h"
#include <stddef.h>

static bool cadence_uart_init(uart_device_t *dev) {
    if (!dev || !dev->base) return false;
    // Skeleton: initialize Cadence UART
    return true;
}

static bool cadence_uart_tx_ready(uart_device_t *dev) {
    (void)dev;
    // Skeleton: check TX readiness
    return true;
}

static void cadence_uart_putc(uart_device_t *dev, char c) {
    (void)dev;
    (void)c;
    // Skeleton: transmit char
}

static size_t cadence_uart_write(uart_device_t *dev, const char *data, size_t len) {
    (void)dev;
    (void)data;
    // Skeleton: transmit sequence
    return len;
}

static void cadence_uart_flush(uart_device_t *dev) {
    (void)dev;
    // Skeleton: flush FIFO
}

static console_caps_t cadence_uart_query_caps(uart_device_t *dev) {
    (void)dev;
    return CON_CAP_WRITE_POLL | CON_CAP_PANIC_SAFE;
}

const uart_driver_ops_t uart_cadence_ops = {
    .init = cadence_uart_init,
    .tx_ready = cadence_uart_tx_ready,
    .putc = cadence_uart_putc,
    .write = cadence_uart_write,
    .flush = cadence_uart_flush,
    .query_caps = cadence_uart_query_caps
};
