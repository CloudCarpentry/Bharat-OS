#include "console/console_backend.h"
#include "console/uart_driver.h"
#include "console/console_discovery.h"
#include "console/console_core.h"
#include <stddef.h>
#include <stdbool.h>

typedef struct {
    uart_device_t *uart;
    bool translate_lf_to_crlf;
    console_output_mode_t mode;
} serial_console_state_t;

static bool serial_init(console_backend_t *backend) {
    if (!backend || !backend->state) return false;
    serial_console_state_t *state = (serial_console_state_t *)backend->state;
    if (!state->uart || !state->uart->ops || !state->uart->ops->init) return false;

    return state->uart->ops->init(state->uart);
}

static size_t serial_write(console_backend_t *backend, const char *data, size_t len) {

    if (!backend || !backend->state || !data) return 0;
    serial_console_state_t *state = (serial_console_state_t *)backend->state;
    uart_device_t *uart = state->uart;

    if (!uart || !uart->ops) return 0;

    if (uart->ops->write) {
        return uart->ops->write(uart, data, len);
    } else if (uart->ops->putc) {
        size_t written = 0;
        for (size_t i = 0; i < len; i++) {
            if (state->translate_lf_to_crlf && data[i] == '\n') {
                uart->ops->putc(uart, '\r');
            }
            uart->ops->putc(uart, data[i]);
            written++;
        }
        return written;
    }

    return 0;
}

static size_t serial_write_atomic(console_backend_t *backend, const char *data, size_t len) {
    // In panic mode, standard putc with polling is usually the safest atomic fallback
    if (!backend || !backend->state || !data) return 0;
    serial_console_state_t *state = (serial_console_state_t *)backend->state;
    uart_device_t *uart = state->uart;

    if (!uart || !uart->ops || !uart->ops->putc) return 0;

    size_t written = 0;
    for (size_t i = 0; i < len; i++) {
        if (state->translate_lf_to_crlf && data[i] == '\n') {
            uart->ops->putc(uart, '\r');
        }
        uart->ops->putc(uart, data[i]);
        written++;
    }
    return written;
}

static void serial_flush(console_backend_t *backend) {
    if (!backend || !backend->state) return;
    serial_console_state_t *state = (serial_console_state_t *)backend->state;
    if (state->uart && state->uart->ops && state->uart->ops->flush) {
        state->uart->ops->flush(state->uart);
    }
}

static void serial_panic_flush(console_backend_t *backend) {
    serial_flush(backend);
}

static console_caps_t serial_query_caps(console_backend_t *backend) {
    if (!backend || !backend->state) return 0;
    serial_console_state_t *state = (serial_console_state_t *)backend->state;
    if (state->uart && state->uart->ops && state->uart->ops->query_caps) {
        return state->uart->ops->query_caps(state->uart) | CON_CAP_VISIBLE_SINK | CON_CAP_EARLY_BOOT | CON_CAP_RUNTIME;
    }
    return CON_CAP_VISIBLE_SINK | CON_CAP_EARLY_BOOT | CON_CAP_RUNTIME;
}

static bool serial_set_mode(console_backend_t *backend, console_output_mode_t mode) {
    if (!backend || !backend->state) return false;
    serial_console_state_t *state = (serial_console_state_t *)backend->state;
    state->mode = mode;
    return true;
}

const console_backend_ops_t serial_console_ops = {
    .init = serial_init,
    .late_init = NULL,
    .shutdown = NULL,
    .write = serial_write,
    .write_atomic = serial_write_atomic,
    .flush = serial_flush,
    .panic_flush = serial_panic_flush,
    .query_caps = serial_query_caps,
    .set_mode = serial_set_mode,
    .clear = NULL,
    .set_cursor = NULL,
    .get_geometry = NULL,
    .poll_input = NULL
};

static console_backend_t g_early_serial_backend;
static serial_console_state_t g_early_serial_state;
static uart_device_t g_early_uart;


#if defined(__aarch64__)
extern const uart_driver_ops_t uart_pl011_ops;
#elif defined(__x86_64__) || defined(__riscv)
extern const uart_driver_ops_t uart_ns16550_ops;
#endif

bool console_serial_register_device(const console_device_desc_t *desc) {
    if (!desc) return false;

    g_early_uart.base = desc->base;
    g_early_uart.reg_shift = desc->reserved0;
#if defined(__aarch64__)
    g_early_uart.ops = &uart_pl011_ops;
#elif defined(__x86_64__) || defined(__riscv)
    g_early_uart.ops = &uart_ns16550_ops;
#else
    return false;
#endif

    g_early_serial_state.uart = &g_early_uart;
    g_early_serial_state.translate_lf_to_crlf = true;

    g_early_serial_backend.name = desc->name;
    g_early_serial_backend.type = CONSOLE_BACKEND_SERIAL;
    g_early_serial_backend.ops = &serial_console_ops;
    g_early_serial_backend.state = &g_early_serial_state;
    g_early_serial_backend.caps = desc->caps;
    g_early_serial_backend.min_level = CONSOLE_LEVEL_INFO;
    g_early_serial_backend.priority = desc->priority;
    g_early_serial_backend.enabled = true;
    g_early_serial_backend.early_ok = true;

    return console_register_backend(&g_early_serial_backend);
}
