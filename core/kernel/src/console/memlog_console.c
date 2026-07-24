#include "console/console_backend.h"
#include "console/console_buffer.h"
#include "console/console_core.h"

#include <stddef.h>
#include <stdbool.h>

typedef struct {
    console_ring_t *ring;
    bool preserve_on_panic;
} memlog_console_state_t;

static memlog_console_state_t g_memlog_state = {
    .ring = NULL, // Points to central ring conceptually, though we don't strictly need a pointer if we use global functions
    .preserve_on_panic = true
};

static bool memlog_init(console_backend_t *backend) {
    if (backend) {
        backend->state = &g_memlog_state;
    }
    return true;
}

static size_t memlog_write(console_backend_t *backend, const char *data, size_t len) {
    (void)backend;
    (void)data;
    // Do nothing: records are already captured in the central ring buffer
    return len;
}

static void memlog_flush(console_backend_t *backend) {
    (void)backend;
}

static void memlog_panic_flush(console_backend_t *backend) {
    (void)backend;
}

static console_caps_t memlog_query_caps(console_backend_t *backend) {
    (void)backend;
    return CON_CAP_EARLY_BOOT |
           CON_CAP_RUNTIME |
           CON_CAP_PANIC_SAFE |
           CON_CAP_REPLAY_SAFE |
           CON_CAP_CRASH_PRESERVE |
           CON_CAP_STORAGE_SINK;
}

static const console_backend_ops_t memlog_ops = {
    .init = memlog_init,
    .late_init = NULL,
    .shutdown = NULL,
    .write = memlog_write,
    .write_atomic = memlog_write,
    .flush = memlog_flush,
    .panic_flush = memlog_panic_flush,
    .query_caps = memlog_query_caps,
    .set_mode = NULL,
    .clear = NULL,
    .set_cursor = NULL,
    .get_geometry = NULL,
    .poll_input = NULL
};

static console_backend_t g_memlog_backend = {
    .name = "memlog",
    .type = CONSOLE_BACKEND_MEMLOG,
    .ops = &memlog_ops,
    .caps = CON_CAP_EARLY_BOOT | CON_CAP_RUNTIME | CON_CAP_PANIC_SAFE | CON_CAP_REPLAY_SAFE | CON_CAP_CRASH_PRESERVE | CON_CAP_STORAGE_SINK,
    .min_level = CONSOLE_LEVEL_TRACE,
    .priority = 100, // Always captures
    .enabled = true,
    .early_ok = true,
    .panic_ok = true,
    .state = &g_memlog_state
};

void console_register_memlog_backend(void);
void console_register_memlog_backend(void) {
    console_register_backend(&g_memlog_backend);
}
