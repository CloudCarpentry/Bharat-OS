#include "bharat/console.h"
#include "hal/hal.h"

static int serial_backend_init(console_backend_t* backend) {
    (void)backend;
    hal_serial_init();
    return 0;
}

static void serial_backend_write(console_backend_t* backend, const char* str, size_t len) {
    (void)backend;
    for (size_t i = 0; i < len; i++) {
        hal_serial_write_char(str[i]);
    }
}

static void serial_backend_flush(console_backend_t* backend) {
    (void)backend;
}

static uint64_t serial_backend_query_caps(console_backend_t* backend) {
    (void)backend;
    return CON_CAP_WRITE_POLLING | CON_CAP_EARLY_BOOT | CON_CAP_CRASH_SAFE | CON_CAP_VT100;
}

static const console_backend_ops_t serial_ops = {
    .init = serial_backend_init,
    .late_init = NULL,
    .write = serial_backend_write,
    .write_atomic = serial_backend_write,
    .flush = serial_backend_flush,
    .panic_flush = serial_backend_flush,
    .set_mode = NULL,
    .query_caps = serial_backend_query_caps,
    .get_geometry = NULL,
    .scroll = NULL,
    .clear = NULL,
    .set_palette = NULL,
    .poll_input = NULL
};

static console_backend_t g_serial_backend = {
    .type = CONSOLE_TYPE_SERIAL,
    .name = "serial",
    .caps = CON_CAP_WRITE_POLLING | CON_CAP_EARLY_BOOT | CON_CAP_CRASH_SAFE | CON_CAP_VT100,
    .enabled = true,
    .min_level = CONSOLE_LEVEL_DEBUG,
    .priority = 50,
    .early_ok = true,
    .panic_ok = true,
    .ops = &serial_ops,
    .priv_data = NULL,
    .next = NULL
};

void console_register_serial_backend(void);
void console_register_serial_backend(void) {
    console_register_backend(&g_serial_backend);
}
