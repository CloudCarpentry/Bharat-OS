#include "bharat/console_discovery.h"
#include <stddef.h>

size_t arch_console_discover(console_device_desc_t* out, size_t max) {
    if (!out || max == 0) return 0;

    // A typical PL011 stub.
    // Actual base would come from DTB in a real system.
    out[0].type = CONSOLE_TYPE_SERIAL;
    out[0].name = "pl011";
    out[0].base = 0x09000000;
    out[0].irq = 1; // Dummy
    out[0].early_ok = true;
    out[0].panic_ok = true;
    out[0].priority = 10;
    out[0].console_caps = CON_CAP_WRITE_POLLING | CON_CAP_EARLY_BOOT | CON_CAP_CRASH_SAFE;

    return 1;
}

console_device_desc_t* arch_console_get_boot_caps(void) { return NULL; }
console_device_desc_t* arch_console_get_firmware_console(void) { return NULL; }

void arch_console_memory_barriers_for_io(void) {
    // __asm__ volatile("dsb sy" ::: "memory");
}

void arch_console_panic_reinit(void) {}
