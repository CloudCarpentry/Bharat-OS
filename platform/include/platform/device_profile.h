#pragma once

#include <stdint.h>
#include <stdbool.h>

/* Neutral descriptor for boot-time UART instances */
typedef struct {
    const char *driver_name; /* e.g., "ns16550", "pl011" */
    uintptr_t base_address;
    uint32_t reg_shift;
    uint32_t reg_width;
    uint32_t input_clock_hz;
    uint32_t baud_rate;
} platform_uart_desc_t;

/* Board hardware description returned by platform */
typedef struct {
    platform_uart_desc_t boot_console;
    bool has_uart;
    bool has_input;
    bool has_display;
} platform_device_profile_t;

/* Platform entry point to query hardware profile */
const platform_device_profile_t *platform_get_device_profile(void);
