#include "hal/hal.h"
#include "debug/early_console.h"
#include "drivers/serial/uart_driver.h"
#include "platform/device_profile.h"
#include "drivers/serial/serial_provider.h"
#include <stdint.h>
#include <stddef.h>

extern const platform_device_profile_t *platform_get_device_profile(void);

void hal_serial_init(void) {
    const platform_device_profile_t *profile = platform_get_device_profile();
    if (profile) {
        uart_device_t *uart = serial_driver_match_boot_console(profile);
        if (uart) {
            early_console_bind(uart);
        }
    }
}

void hal_serial_write_char(char c) {
    early_console_putc(c);
}

void hal_serial_write(const char *s) {
    if (!s) return;
    while (*s != '\0') {
        early_console_putc(*s);
        s++;
    }
}

void hal_serial_write_hex(uint64_t val) {
    char buf[17];
    buf[16] = '\0';
    for (int i = 15; i >= 0; i--) {
        uint8_t nibble = val & 0xF;
        buf[i] = nibble < 10 ? '0' + nibble : 'a' + (nibble - 10);
        val >>= 4;
    }
    hal_serial_write("0x");
    hal_serial_write(buf);
}

int hal_serial_read_char(void) {
    // Explicitly unsupported: Serial read path should be implemented in proper
    // low-level console driver, not as a compat shim here.
    return -1;
}
