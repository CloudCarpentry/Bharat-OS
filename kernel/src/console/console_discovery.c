#include "console/console_discovery.h"
#include "platform/device_profile.h"
#include "drivers/serial/serial_provider.h"

size_t console_discover_devices(console_device_desc_t *out, size_t max_count) {
    if (!out || max_count == 0) {
        return 0;
    }

    const platform_device_profile_t *profile = platform_get_device_profile();
    if (!profile || !profile->has_uart) {
        return 0;
    }

    uart_device_t *boot_uart = serial_driver_match_boot_console(profile);
    if (!boot_uart) {
        return 0;
    }

    out[0].type = CONSOLE_BACKEND_SERIAL;
    out[0].early_ok = 1;
    out[0].panic_ok = 1;
    out[0].reserved0 = 0;
    out[0].priority = 100;
    out[0].base = (uintptr_t)boot_uart->base;
    out[0].irq = 0;
    out[0].caps = CON_CAP_EARLY_BOOT | CON_CAP_RUNTIME | CON_CAP_PANIC_SAFE | CON_CAP_WRITE_POLL | CON_CAP_VISIBLE_SINK;
    out[0].name = profile->boot_console.driver_name ? profile->boot_console.driver_name : "boot-uart";
    out[0].opaque = (void *)boot_uart;

    return 1;
}
