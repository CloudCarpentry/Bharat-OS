#include "bharat/console.h"
#include "console_internal.h"
#include <stdarg.h>
#include <stdbool.h>

// Simple spinlock for console output (early boot safe)
static volatile uint32_t console_lock = 0;

static void spin_lock(volatile uint32_t* lock) {
    while (__sync_lock_test_and_set(lock, 1)) {
        while (*lock); // Pause for performance
    }
}

static void spin_unlock(volatile uint32_t* lock) {
    __sync_lock_release(lock);
}

// Global list of backends
static console_backend_t* g_backends = NULL;
static bool g_in_panic = false;

static size_t string_length(const char* str) {
    size_t len = 0;
    while (str[len]) len++;
    return len;
}

void console_init(void) {
    g_backends = NULL;
    g_in_panic = false;
    console_lock = 0;
}

int console_register_backend(console_backend_t* backend) {
    if (!backend) return -1;

    spin_lock(&console_lock);

    if (backend->init) {
        if (backend->init() != 0) {
            spin_unlock(&console_lock);
            return -1; // Init failed
        }
    }

    // Add to head of list
    backend->next = g_backends;
    g_backends = backend;

    spin_unlock(&console_lock);
    return 0;
}

void console_write_raw(const char* str) {
    size_t len = string_length(str);

    if (!g_in_panic) {
        spin_lock(&console_lock);
    }

    console_backend_t* current = g_backends;
    while (current) {
        if (current->write) {
            current->write(str, len);
        }
        current = current->next;
    }

    if (!g_in_panic) {
        spin_unlock(&console_lock);
    }
}

// Minimal formatted logging that prefixes severity
void console_log(console_log_level_t level, const char* fmt, ...) {
    const char* prefix = "";
    switch (level) {
        case CONSOLE_LEVEL_DEBUG: prefix = "[DEBUG] "; break;
        case CONSOLE_LEVEL_INFO:  prefix = "[INFO]  "; break;
        case CONSOLE_LEVEL_WARN:  prefix = "[WARN]  "; break;
        case CONSOLE_LEVEL_ERROR: prefix = "[ERROR] "; break;
        case CONSOLE_LEVEL_PANIC: prefix = "[PANIC] "; g_in_panic = true; break;
    }

    if (!g_in_panic) {
        spin_lock(&console_lock);
    }

    // Write prefix
    size_t prefix_len = string_length(prefix);
    console_backend_t* current = g_backends;
    while (current) {
        if (current->write) {
            current->write(prefix, prefix_len);
        }
        current = current->next;
    }

    // Process input character by character
    const char* p = fmt;
    while (*p) {
        char c = *p;
        // Broadcast single byte
        current = g_backends;
        while (current) {
            if (current->write) {
                current->write(&c, 1);
            }
            current = current->next;
        }
        p++;
    }

    if (!g_in_panic) {
        spin_unlock(&console_lock);
    }
}
