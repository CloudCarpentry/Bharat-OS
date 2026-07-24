#include "console/console_discovery.h"

size_t arch_console_discover(console_device_desc_t *out, size_t max_count) {
    if (!out || max_count == 0) return 0;

    // ARM64 PL011 Serial Example
    out[0].type = CONSOLE_BACKEND_SERIAL;
    out[0].early_ok = 1;
    out[0].panic_ok = 1;
    out[0].reserved0 = 0;
    out[0].priority = 100;
    out[0].base = 0x9000000; // QEMU Virt PL011 Base
    out[0].irq = 33;
    out[0].caps = CON_CAP_EARLY_BOOT | CON_CAP_RUNTIME | CON_CAP_PANIC_SAFE | CON_CAP_WRITE_POLL;
    out[0].name = "PL011";
    out[0].opaque = NULL;

    return 1;
}
