#include "console/uart_driver.h"
#include "../../../drivers/serial/pl011/pl011.h"

static uart_device_t g_boot_uart = {
    .ops = &uart_pl011_ops,
    .base = 0x09000000UL, // QEMU virt ARM64 PL011 base address
    .reg_shift = 0,
    .reg_width = 4,
    .input_clock_hz = 24000000,
    .baud_rate = 115200,
    .opaque = NULL
};

uart_device_t *platform_get_boot_uart(void) {
    return &g_boot_uart;
}
