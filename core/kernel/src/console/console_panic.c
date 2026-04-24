#include "console/console_panic.h"
#include "console/console_core.h"
#include "console/console_router.h"

extern console_global_state_t g_console_state;

void console_enter_panic(void) {
    if (g_console_state.in_panic) {
        return; // Already in panic
    }

    g_console_state.in_panic = true;
    g_console_state.phase = CONSOLE_PHASE_PANIC;
}

void console_panic_flush_backends(void) {
    for (console_index_t i = 0; i < g_console_state.backend_count; i++) {
        console_backend_t *backend = g_console_state.backends[i];
        if (!backend || !backend->enabled) continue;

        if (console_backend_supports_phase(backend, CONSOLE_PHASE_PANIC)) {
            if (backend->ops && backend->ops->panic_flush) {
                backend->ops->panic_flush(backend);
            } else if (backend->ops && backend->ops->flush) {
                backend->ops->flush(backend);
            }
        }
    }
}
