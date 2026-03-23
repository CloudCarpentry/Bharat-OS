#include "console/uart_driver.h"
#include "../../../drivers/serial/ns16550/ns16550.h"

static uart_device_t g_boot_uart = {
    .ops = &uart_ns16550_ops,
    .base = 0x3F8, // x86 PC COM1 base port (I/O, not MMIO; requires driver to use inb/outb if configured this way, but for now ns16550 is MMIO in the driver. WAIT!)
    .reg_shift = 0,
    .reg_width = 1,
    .input_clock_hz = 115200 * 16,
    .baud_rate = 115200,
    .opaque = NULL
};

uart_device_t *platform_get_boot_uart(void) {
    return &g_boot_uart;
}
