#include "platform/device_profile.h"
#include <stddef.h>

static const platform_device_profile_t g_device_profile = {
    .boot_console = {
        .driver_name = "ns16550",
        .base_address = 0x10000000UL, // QEMU virt RISC-V NS16550 base address
        .reg_shift = 0,
        .reg_width = 1,
        .input_clock_hz = 0, // Usually unused for basic init, or pre-configured by SBI
        .baud_rate = 115200
    },
    .has_uart = true,
    .has_input = true,
    .has_display = true
};

const platform_device_profile_t *platform_get_device_profile(void) {
    return &g_device_profile;
}
