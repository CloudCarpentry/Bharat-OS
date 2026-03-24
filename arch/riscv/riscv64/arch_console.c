#include "console/console_discovery.h"
#include "console/console_types.h"
#include <stddef.h>

#ifndef BHARAT_ARCH_SERIAL_BASE
#define BHARAT_ARCH_SERIAL_BASE 0x10000000
#endif

size_t arch_console_discover(console_device_desc_t* out, size_t max) {
    if (!out || max == 0) return 0;

    out[0].type = CONSOLE_BACKEND_SERIAL;
    out[0].name = "uart8250";
    out[0].caps = CON_CAP_WRITE_POLL | CON_CAP_EARLY_BOOT | CON_CAP_PANIC_SAFE;
    out[0].base = BHARAT_ARCH_SERIAL_BASE;
    out[0].irq = 10;
    out[0].early_ok = 1;
    out[0].panic_ok = 1;
    out[0].priority = 100;
    out[0].opaque = NULL;

    return 1;
}

console_device_desc_t* arch_console_get_boot_caps(void) { return NULL; }
console_device_desc_t* arch_console_get_firmware_console(void) { return NULL; }
void arch_console_memory_barriers_for_io(void) {}
void arch_console_panic_reinit(void) {}
