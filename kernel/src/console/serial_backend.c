#include "bharat/console.h"
#include "hal/hal.h"

// Backend interface implementations

static int serial_backend_init(void) {
    // Rely on HAL to initialize serial
    hal_serial_init();
    return 0;
}

static void serial_backend_write(const char* str, size_t len) {
    for (size_t i = 0; i < len; i++) {
        hal_serial_write_char(str[i]);
    }
}

static void serial_backend_flush(void) {
    // Some HAL serial implementations might support a flush,
    // but typically they are blocking/polled writes.
}

static console_backend_t g_serial_backend = {
    .type = CONSOLE_BACKEND_SERIAL,
    .name = "serial",
    .init = serial_backend_init,
    .write = serial_backend_write,
    .flush = serial_backend_flush,
    .next = NULL
};

void console_register_serial_backend(void) {
    console_register_backend(&g_serial_backend);
}
