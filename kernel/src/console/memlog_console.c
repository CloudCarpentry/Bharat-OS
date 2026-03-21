#include "bharat/console.h"
#include <stddef.h>

#define MEMLOG_BUFFER_SIZE 4096

static char memlog_buffer[MEMLOG_BUFFER_SIZE];
static size_t memlog_head = 0;
static size_t memlog_tail = 0;
static bool memlog_wrapped = false;

static int memlog_init(console_backend_t* backend) {
    (void)backend;
    memlog_head = 0;
    memlog_tail = 0;
    memlog_wrapped = false;
    return 0;
}

static void memlog_write(console_backend_t* backend, const char* str, size_t len) {
    (void)backend;
    for (size_t i = 0; i < len; i++) {
        memlog_buffer[memlog_head] = str[i];
        memlog_head = (memlog_head + 1) % MEMLOG_BUFFER_SIZE;
        if (memlog_head == memlog_tail) {
            memlog_tail = (memlog_tail + 1) % MEMLOG_BUFFER_SIZE;
            memlog_wrapped = true;
        }
    }
}

static void memlog_flush(console_backend_t* backend) {
    (void)backend;
    // Memory ring requires no explicit flush to hardware
}

static void memlog_panic_flush(console_backend_t* backend) {
    (void)backend;
    // Memory ring requires no explicit flush to hardware
}

static uint64_t memlog_query_caps(console_backend_t* backend) {
    (void)backend;
    return CON_CAP_WRITE_POLLING | CON_CAP_EARLY_BOOT | CON_CAP_CRASH_SAFE | CON_CAP_SCROLLBACK;
}

static const console_backend_ops_t memlog_ops = {
    .init = memlog_init,
    .late_init = NULL,
    .write = memlog_write,
    .write_atomic = memlog_write,
    .flush = memlog_flush,
    .panic_flush = memlog_panic_flush,
    .set_mode = NULL,
    .query_caps = memlog_query_caps,
    .get_geometry = NULL,
    .scroll = NULL,
    .clear = NULL,
    .set_palette = NULL,
    .poll_input = NULL
};

static console_backend_t g_memlog_backend = {
    .type = CONSOLE_TYPE_MEMORY_LOG,
    .name = "memlog",
    .caps = CON_CAP_WRITE_POLLING | CON_CAP_EARLY_BOOT | CON_CAP_CRASH_SAFE | CON_CAP_SCROLLBACK,
    .enabled = true,
    .min_level = CONSOLE_LEVEL_DEBUG,
    .priority = 100, // Always captures
    .early_ok = true,
    .panic_ok = true,
    .ops = &memlog_ops,
    .priv_data = NULL,
    .next = NULL
};

void console_register_memlog_backend(void);
void console_register_memlog_backend(void) {
    console_register_backend(&g_memlog_backend);
}

// Function to replay logs to a newly registered backend
void memlog_replay_to(console_backend_t* backend) {
    if (!backend || !backend->ops || !backend->ops->write) return;

    size_t current = memlog_tail;
    bool wrapped = memlog_wrapped;

    if (wrapped) {
        // Read from tail to end of buffer
        backend->ops->write(backend, &memlog_buffer[current], MEMLOG_BUFFER_SIZE - current);
        current = 0;
    }

    // Read remaining
    if (memlog_head > current) {
         backend->ops->write(backend, &memlog_buffer[current], memlog_head - current);
    }
}
