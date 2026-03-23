#include "console/uart_driver.h"
#include "../../../drivers/serial/ns16550/ns16550.h"

static uart_device_t g_boot_uart = {
    .ops = &uart_ns16550_ops,
    .base = 0x10000000UL, // QEMU virt RISC-V NS16550 base address
    .reg_shift = 0,
    .reg_width = 1,
    .input_clock_hz = 0, // Usually unused for basic init, or pre-configured by SBI
    .baud_rate = 115200,
    .opaque = NULL
};

uart_device_t *platform_get_boot_uart(void) {
    return &g_boot_uart;
}
