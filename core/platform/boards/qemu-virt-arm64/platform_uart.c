#include "platform/device_profile.h"
#include <stddef.h>

static const platform_device_profile_t g_device_profile = {
    .boot_console = {
        .driver_name = "pl011",
        .base_address = 0x09000000UL, // QEMU virt ARM64 PL011 base address
        .reg_shift = 0,
        .reg_width = 4,
        .input_clock_hz = 24000000,
        .baud_rate = 115200
    },
    .has_uart = true,
    .has_input = true,
    .has_display = true
};

const platform_device_profile_t *platform_get_device_profile(void) {
    return &g_device_profile;
}
