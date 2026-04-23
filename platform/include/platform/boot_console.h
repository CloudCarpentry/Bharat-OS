#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "console/console_caps.h"
#include "console/console_types.h"
#include "drivers/serial/uart_driver.h"

/*
 * Canonical platform-resolved boot-console descriptor.
 * This is the single source consumed by console discovery/bind code.
 */
typedef struct {
    bool valid;
    const char *name;
    uint8_t type;
    uint8_t early_ok;
    uint8_t panic_ok;
    uint32_t priority;
    console_caps_t caps;
    const uart_device_t *uart;
} platform_boot_console_desc_t;

bool platform_get_boot_console_desc(platform_boot_console_desc_t *out);
