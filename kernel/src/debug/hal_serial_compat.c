#include "hal/hal.h"
#include "debug/early_console.h"
#include "console/uart_driver.h"
#include <stdint.h>
#include <stddef.h>

extern uart_device_t *platform_get_boot_uart(void);

void hal_serial_init(void) {
    uart_device_t *uart = platform_get_boot_uart();
    if (uart) {
        early_console_bind(uart);
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
    // Left unimplemented for basic early console output
    return -1;
}
