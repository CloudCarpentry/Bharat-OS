#include "console/console_discovery.h"
#include "platform/boot_console.h"

size_t console_discover_devices(console_device_desc_t *out, size_t max_count) {
    if (!out || max_count == 0) {
        return 0;
    }

    platform_boot_console_desc_t boot_console;
    if (!platform_get_boot_console_desc(&boot_console) || !boot_console.valid || !boot_console.uart) {
        return 0;
    }

    out[0].type = boot_console.type;
    out[0].early_ok = boot_console.early_ok;
    out[0].panic_ok = boot_console.panic_ok;
    out[0].reserved0 = 0;
    out[0].priority = boot_console.priority;
    out[0].base = (uintptr_t)boot_console.uart->base;
    out[0].irq = 0;
    out[0].caps = boot_console.caps;
    out[0].name = boot_console.name;
    out[0].opaque = (void *)boot_console.uart;

    return 1;
}
