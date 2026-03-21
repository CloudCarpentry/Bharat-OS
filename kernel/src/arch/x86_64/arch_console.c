#include "bharat/console_discovery.h"
#include <stddef.h>

size_t arch_console_discover(console_device_desc_t* out, size_t max) {
    if (!out || max == 0) return 0;

    out[0].type = CONSOLE_TYPE_SERIAL;
    out[0].name = "com1";
    out[0].base = 0x3F8;
    out[0].irq = 4;
    out[0].early_ok = true;
    out[0].panic_ok = true;
    out[0].priority = 10;
    out[0].console_caps = CON_CAP_WRITE_POLLING | CON_CAP_EARLY_BOOT | CON_CAP_CRASH_SAFE;

    if (max > 1) {
        out[1].type = CONSOLE_TYPE_VGA_TEXT;
        out[1].name = "vga_text";
        out[1].base = 0xB8000;
        out[1].early_ok = true;
        out[1].panic_ok = true;
        out[1].priority = 5;
        out[1].console_caps = CON_CAP_WRITE_POLLING | CON_CAP_EARLY_BOOT | CON_CAP_COLOR | CON_CAP_CURSOR_ADDR | CON_CAP_CRASH_SAFE;
        return 2;
    }

    return 1;
}

console_device_desc_t* arch_console_get_boot_caps(void) { return NULL; }
console_device_desc_t* arch_console_get_firmware_console(void) { return NULL; }
void arch_console_memory_barriers_for_io(void) {}
void arch_console_panic_reinit(void) {}
