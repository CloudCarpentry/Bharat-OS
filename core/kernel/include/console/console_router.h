#pragma once

#include "console_backend.h"
#include "console_record.h"

bool console_backend_supports_phase(const console_backend_t *backend, console_phase_t phase);
bool console_backend_accepts_level(const console_backend_t *backend, console_level_t level);
bool console_backend_is_visible_sink(const console_backend_t *backend);
bool console_backend_is_storage_sink(const console_backend_t *backend);
void console_route_record(const console_record_t *rec);
void console_route_record_panic(const console_record_t *rec);
