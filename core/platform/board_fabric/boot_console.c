#include "platform/boot_console.h"

#include "platform/device_profile.h"
#include "drivers/serial/serial_provider.h"

bool platform_get_boot_console_desc(platform_boot_console_desc_t *out) {
    if (!out) {
        return false;
    }

    out->valid = false;
    out->name = NULL;
    out->type = CONSOLE_BACKEND_NONE;
    out->early_ok = 0;
    out->panic_ok = 0;
    out->priority = 0;
    out->caps = 0;
    out->uart = NULL;

    const platform_device_profile_t *profile = platform_get_device_profile();
    if (!profile || !profile->has_uart) {
        return false;
    }

    uart_device_t *boot_uart = serial_driver_match_boot_console(profile);
    if (!boot_uart) {
        return false;
    }

    out->valid = true;
    out->name = profile->boot_console.driver_name ? profile->boot_console.driver_name : "boot-uart";
    out->type = CONSOLE_BACKEND_SERIAL;
    out->early_ok = 1;
    out->panic_ok = 1;
    out->priority = 100;
    out->caps = CON_CAP_EARLY_BOOT | CON_CAP_RUNTIME | CON_CAP_PANIC_SAFE | CON_CAP_WRITE_POLL | CON_CAP_VISIBLE_SINK;
    out->uart = boot_uart;

    return true;
}
