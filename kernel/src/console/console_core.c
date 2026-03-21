#include "bharat/console.h"
#include <stdarg.h>
#include <stdbool.h>

// Early boot lock (spinlock, simple fallback)
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
static console_phase_t g_phase = CONSOLE_PHASE_EARLY;

static size_t string_length(const char* str) {
    size_t len = 0;
    while (str[len]) len++;
    return len;
}

console_phase_t console_get_phase(void) {
    return g_phase;
}

void console_init_early(void) {
    g_backends = NULL;
    g_phase = CONSOLE_PHASE_EARLY;
    console_lock = 0;
}

void console_init_runtime(void) {
    spin_lock(&console_lock);
    g_phase = CONSOLE_PHASE_RUNTIME;

    console_backend_t* current = g_backends;
    while (current) {
        if (current->ops && current->ops->late_init) {
            current->ops->late_init(current);
        }
        current = current->next;
    }

    // Future: Replay memory buffer

    spin_unlock(&console_lock);
}

void console_panic_mode(void) {
    g_phase = CONSOLE_PHASE_PANIC;
}

int console_register_backend(console_backend_t* backend) {
    if (!backend || !backend->ops) return -1;

    // Filter early registration
    if (g_phase == CONSOLE_PHASE_EARLY && !backend->early_ok) {
        return -2;
    }

    spin_lock(&console_lock);

    if (backend->ops->init) {
        if (backend->ops->init(backend) != 0) {
            spin_unlock(&console_lock);
            return -1; // Init failed
        }
    }

    // Default configuration if missing
    if (!backend->enabled) backend->enabled = true;

    // Add to list
    backend->next = g_backends;
    g_backends = backend;

    spin_unlock(&console_lock);
    return 0;
}

static void backend_emit(console_backend_t* current, console_log_level_t level, const char* str, size_t len, bool atomic) {
    if (!current->enabled) return;
    if (level < current->min_level) return;
    if (g_phase == CONSOLE_PHASE_PANIC && !current->panic_ok) return;

    if (g_phase == CONSOLE_PHASE_PANIC || atomic) {
        if (current->ops->write_atomic) {
            current->ops->write_atomic(current, str, len);
        } else if (current->ops->write && current->panic_ok) {
             current->ops->write(current, str, len);
        }
    } else {
        if (current->ops->write) {
            current->ops->write(current, str, len);
        }
    }
}

void console_write_raw(const char* str) {
    size_t len = string_length(str);

    if (g_phase != CONSOLE_PHASE_PANIC) spin_lock(&console_lock);

    console_backend_t* current = g_backends;
    while (current) {
        backend_emit(current, CONSOLE_LEVEL_DEBUG, str, len, false);
        current = current->next;
    }

    if (g_phase != CONSOLE_PHASE_PANIC) spin_unlock(&console_lock);
}

void console_write_atomic(const char* str) {
    size_t len = string_length(str);
    console_backend_t* current = g_backends;
    while (current) {
        backend_emit(current, CONSOLE_LEVEL_DEBUG, str, len, true);
        current = current->next;
    }
}

// Format severity to backend output
void console_log(console_log_level_t level, const char* fmt, ...) {
    const char* prefix = "";
    switch (level) {
        case CONSOLE_LEVEL_DEBUG: prefix = "[DEBUG] "; break;
        case CONSOLE_LEVEL_INFO:  prefix = "[INFO]  "; break;
        case CONSOLE_LEVEL_WARN:  prefix = "[WARN]  "; break;
        case CONSOLE_LEVEL_ERROR: prefix = "[ERROR] "; break;
        case CONSOLE_LEVEL_PANIC: prefix = "[PANIC] "; break;
    }

    if (level == CONSOLE_LEVEL_PANIC) {
        console_panic_mode();
    }

    if (g_phase != CONSOLE_PHASE_PANIC) spin_lock(&console_lock);

    size_t prefix_len = string_length(prefix);

    // Simplistic formatting for skeleton
    console_backend_t* current = g_backends;
    while (current) {
        backend_emit(current, level, prefix, prefix_len, false);
        current = current->next;
    }

    // Process characters
    // In a real implementation this uses vsnprintf to a temporary buffer.
    const char* p = fmt;
    while (*p) {
        char c = *p;
        current = g_backends;
        while (current) {
            backend_emit(current, level, &c, 1, false);
            current = current->next;
        }
        p++;
    }

    if (g_phase != CONSOLE_PHASE_PANIC) spin_unlock(&console_lock);
}
