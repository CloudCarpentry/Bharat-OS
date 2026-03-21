#include "console/console_core.h"
#include "console/console_buffer.h"
#include "console/console_router.h"
#include "console/console_panic.h"
#include "console/console_discovery.h"
#include "console/console_policy.h"
#include "console_format.h"

#include <stdarg.h>
#include <stdbool.h>

console_global_state_t g_console_state;
static volatile uint32_t console_lock = 0;

static void spin_lock(volatile uint32_t* lock) {
    while (__sync_lock_test_and_set(lock, 1)) {
        while (*lock);
    }
}

static void spin_unlock(volatile uint32_t* lock) {
    __sync_lock_release(lock);
}

size_t string_length(const char* str) {
    size_t len = 0;
    while (str && str[len]) len++;
    return len;
}

console_phase_t console_current_phase(void) {
    return g_console_state.phase;
}

void console_early_init(void) {
    g_console_state.phase = CONSOLE_PHASE_EARLY;
    g_console_state.in_panic = false;
    g_console_state.backend_count = 0;
    g_console_state.runtime_primary = NULL;
    g_console_state.panic_primary = NULL;
    console_lock = 0;

    console_buffer_init();

    extern void console_register_memlog_backend(void);
    console_register_memlog_backend();

    // Discover devices and set policy
    console_device_desc_t devs[CONSOLE_MAX_BACKENDS];
    size_t count = console_discover_devices(devs, CONSOLE_MAX_BACKENDS);

    console_policy_decision_t decision;
    console_choose_policy(devs, count, &decision);

    // Later: Actual backend instances for serial/fb could be instantiated based on decision
}

void console_runtime_init(void) {
    spin_lock(&console_lock);
    g_console_state.phase = CONSOLE_PHASE_RUNTIME;

    // Discover runtime devices and update policy
    console_device_desc_t devs[CONSOLE_MAX_BACKENDS];
    size_t count = console_discover_devices(devs, CONSOLE_MAX_BACKENDS);

    console_policy_decision_t decision;
    console_choose_policy(devs, count, &decision);

    for (console_index_t i = 0; i < g_console_state.backend_count; i++) {
        console_backend_t *backend = g_console_state.backends[i];
        if (backend && backend->ops && backend->ops->late_init) {
            backend->ops->late_init(backend);
        }
    }

    // Replay early buffer to newly active visible runtime backends
    for (console_index_t i = 0; i < g_console_state.backend_count; i++) {
        console_backend_t *backend = g_console_state.backends[i];
        if (backend && backend->enabled && console_backend_is_visible_sink(backend)) {
            if (backend->caps & CON_CAP_REPLAY_SAFE) {
                console_buffer_replay_to_backend(backend, false);
            }
        }
    }

    spin_unlock(&console_lock);
}

bool console_register_backend(console_backend_t* backend) {
    if (!backend || !backend->ops) return false;

    if (g_console_state.phase == CONSOLE_PHASE_EARLY && !console_backend_supports_phase(backend, CONSOLE_PHASE_EARLY)) {
        return false;
    }

    spin_lock(&console_lock);

    if (g_console_state.backend_count >= CONSOLE_MAX_BACKENDS) {
        spin_unlock(&console_lock);
        return false;
    }

    if (backend->ops->init) {
        if (!backend->ops->init(backend)) {
            spin_unlock(&console_lock);
            return false;
        }
    }

    if (!backend->enabled) backend->enabled = true;

    g_console_state.backends[g_console_state.backend_count++] = backend;

    spin_unlock(&console_lock);
    return true;
}

bool console_activate_backend(console_backend_t* backend) {
    if (!backend) return false;
    backend->enabled = true;
    return true;
}

void console_vlog(console_log_level_t level, const char *fmt, va_list ap) {
    if (level == CONSOLE_LEVEL_PANIC) {
        console_enter_panic();
    }

    if (!g_console_state.in_panic) spin_lock(&console_lock);

    console_record_t rec = {0};
    rec.level = level;
    // timestamp could be added here if available
    // rec.timestamp_ns = get_time_ns();

    const char* prefix = "";
    switch (level) {
        case CONSOLE_LEVEL_DEBUG: prefix = "[DEBUG] "; break;
        case CONSOLE_LEVEL_INFO:  prefix = "[INFO]  "; break;
        case CONSOLE_LEVEL_WARN:  prefix = "[WARN]  "; break;
        case CONSOLE_LEVEL_ERROR: prefix = "[ERROR] "; break;
        case CONSOLE_LEVEL_PANIC: prefix = "[PANIC] "; break;
        default: prefix = ""; break;
    }

    size_t prefix_len = string_length(prefix);
    char temp_buf[CONSOLE_MAX_RECORD_TEXT];
    size_t temp_len = 0;

    for (size_t i = 0; i < prefix_len && i < CONSOLE_MAX_RECORD_TEXT - 1; i++) {
        temp_buf[i] = prefix[i];
        temp_len++;
    }

    int written = console_vsnprintf(temp_buf + temp_len, CONSOLE_MAX_RECORD_TEXT - temp_len, fmt, ap);
    if (written > 0) {
        temp_len += written;
    }

    if (temp_len >= CONSOLE_MAX_RECORD_TEXT) {
        temp_len = CONSOLE_MAX_RECORD_TEXT - 1;
        rec.flags |= CON_RECORD_FLAG_TRUNCATED;
    }
    temp_buf[temp_len] = '\0';

    for (size_t i = 0; i < temp_len; i++) {
        rec.text[i] = temp_buf[i];
    }
    rec.text_len = (console_len_t)temp_len;

    if (g_console_state.in_panic) {
        rec.flags |= CON_RECORD_FLAG_PANIC_PATH;
    }

    console_buffer_push(&rec);

    if (g_console_state.in_panic) {
        console_route_record_panic(&rec);
    } else {
        console_route_record(&rec);
    }

    if (!g_console_state.in_panic) spin_unlock(&console_lock);
}

void console_log(console_log_level_t level, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    console_vlog(level, fmt, ap);
    va_end(ap);
}

void console_write_raw(const char *data, size_t len) {
    if (!g_console_state.in_panic) spin_lock(&console_lock);

    console_record_t rec = {0};
    rec.level = CONSOLE_LEVEL_INFO;
    rec.flags |= CON_RECORD_FLAG_RAW_WRITE;

    if (len >= CONSOLE_MAX_RECORD_TEXT) {
        len = CONSOLE_MAX_RECORD_TEXT - 1;
        rec.flags |= CON_RECORD_FLAG_TRUNCATED;
    }

    for (size_t i = 0; i < len; i++) {
        rec.text[i] = data[i];
    }
    rec.text_len = (console_len_t)len;

    console_buffer_push(&rec);

    if (g_console_state.in_panic) {
        console_route_record_panic(&rec);
    } else {
        console_route_record(&rec);
    }

    if (!g_console_state.in_panic) spin_unlock(&console_lock);
}
