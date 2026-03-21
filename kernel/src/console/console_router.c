#include "console/console_router.h"
#include "console/console_core.h"
#include <stddef.h>

extern console_global_state_t g_console_state;

bool console_backend_supports_phase(const console_backend_t *backend, console_phase_t phase) {
    if (!backend) return false;

    if (phase == CONSOLE_PHASE_EARLY) {
        return backend->early_ok || (backend->caps & CON_CAP_EARLY_BOOT);
    } else if (phase == CONSOLE_PHASE_RUNTIME) {
        return (backend->caps & CON_CAP_RUNTIME) != 0;
    } else if (phase == CONSOLE_PHASE_PANIC) {
        return backend->panic_ok || (backend->caps & CON_CAP_PANIC_SAFE);
    }
    return false;
}

bool console_backend_accepts_level(const console_backend_t *backend, console_level_t level) {
    if (!backend) return false;
    return level >= backend->min_level;
}

bool console_backend_is_visible_sink(const console_backend_t *backend) {
    if (!backend) return false;
    return (backend->caps & CON_CAP_VISIBLE_SINK) != 0;
}

bool console_backend_is_storage_sink(const console_backend_t *backend) {
    if (!backend) return false;
    return (backend->caps & CON_CAP_STORAGE_SINK) != 0;
}

void console_route_record(const console_record_t *rec) {
    if (!rec) return;

    for (console_index_t i = 0; i < g_console_state.backend_count; i++) {
        console_backend_t *backend = g_console_state.backends[i];
        if (!backend || !backend->enabled) continue;

        if (!console_backend_supports_phase(backend, g_console_state.phase)) continue;
        if (!console_backend_accepts_level(backend, rec->level)) continue;

        if (backend->ops && backend->ops->write) {
            backend->ops->write(backend, rec->text, rec->text_len);
        }
    }
}

void console_route_record_panic(const console_record_t *rec) {
    if (!rec) return;

    for (console_index_t i = 0; i < g_console_state.backend_count; i++) {
        console_backend_t *backend = g_console_state.backends[i];
        if (!backend || !backend->enabled) continue;

        if (!console_backend_supports_phase(backend, CONSOLE_PHASE_PANIC)) continue;
        if (!console_backend_accepts_level(backend, rec->level)) continue;

        if (backend->ops) {
            if (backend->ops->write_atomic) {
                backend->ops->write_atomic(backend, rec->text, rec->text_len);
            } else if (backend->ops->write) {
                backend->ops->write(backend, rec->text, rec->text_len);
            }
        }
    }
}
