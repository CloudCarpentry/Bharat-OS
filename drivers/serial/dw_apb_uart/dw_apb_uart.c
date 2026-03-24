#include "dw_apb_uart.h"
#include <stddef.h>

static bool dw_apb_uart_init(uart_device_t *dev) {
    if (!dev || !dev->base) return false;
    // Skeleton: initialize DW APB UART
    return true;
}

static bool dw_apb_uart_tx_ready(uart_device_t *dev) {
    (void)dev;
    // Skeleton: check TX readiness
    return true;
}

static void dw_apb_uart_putc(uart_device_t *dev, char c) {
    (void)dev;
    (void)c;
    // Skeleton: transmit char
}

static size_t dw_apb_uart_write(uart_device_t *dev, const char *data, size_t len) {
    (void)dev;
    (void)data;
    // Skeleton: transmit sequence
    return len;
}

static void dw_apb_uart_flush(uart_device_t *dev) {
    (void)dev;
    // Skeleton: flush FIFO
}

static console_caps_t dw_apb_uart_query_caps(uart_device_t *dev) {
    (void)dev;
    return CON_CAP_WRITE_POLL | CON_CAP_PANIC_SAFE;
}

const uart_driver_ops_t uart_dw_apb_ops = {
    .init = dw_apb_uart_init,
    .tx_ready = dw_apb_uart_tx_ready,
    .putc = dw_apb_uart_putc,
    .write = dw_apb_uart_write,
    .flush = dw_apb_uart_flush,
    .query_caps = dw_apb_uart_query_caps
};
