#pragma once

#include <stdarg.h>

#include "console_backend.h"
#include "console_record.h"
#include "console_types.h"

#define CONSOLE_MAX_BACKENDS 16

typedef struct {
    console_phase_t phase;
    bool in_panic;
    console_backend_t *backends[CONSOLE_MAX_BACKENDS];
    console_index_t backend_count;
    console_backend_t *runtime_primary;
    console_backend_t *panic_primary;
} console_global_state_t;

void console_early_init(void);
void console_runtime_init(void);
void console_enter_panic(void);

bool console_register_backend(console_backend_t *backend);
bool console_activate_backend(console_backend_t *backend);

void console_log(console_log_level_t level, const char *fmt, ...);
void console_vlog(console_log_level_t level, const char *fmt, va_list ap);

void console_write_raw(const char *data, size_t len);
console_phase_t console_current_phase(void);
