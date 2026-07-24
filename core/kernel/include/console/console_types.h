#pragma once

#include "console_base_types.h"

typedef enum {
    CONSOLE_PHASE_EARLY = 0,
    CONSOLE_PHASE_RUNTIME = 1,
    CONSOLE_PHASE_PANIC = 2
} console_phase_t;

typedef enum {
    CONSOLE_BACKEND_NONE = 0,
    CONSOLE_BACKEND_MEMLOG = 1,
    CONSOLE_BACKEND_SERIAL = 2,
    CONSOLE_BACKEND_FRAMEBUFFER = 3,
    CONSOLE_BACKEND_FIRMWARE = 4,
    CONSOLE_BACKEND_DEBUGPORT = 5,
    CONSOLE_BACKEND_SBI = 6,
    CONSOLE_BACKEND_VGA_TEXT = 7
} console_backend_type_t;

typedef enum {
    CONSOLE_LEVEL_TRACE = 0,
    CONSOLE_LEVEL_DEBUG = 1,
    CONSOLE_LEVEL_INFO  = 2,
    CONSOLE_LEVEL_WARN  = 3,
    CONSOLE_LEVEL_ERROR = 4,
    CONSOLE_LEVEL_FATAL = 5,
    CONSOLE_LEVEL_PANIC = 6
} console_log_level_t;

typedef enum {
    CONSOLE_OUTPUT_PLAIN = 0,
    CONSOLE_OUTPUT_VT100 = 1,
    CONSOLE_OUTPUT_FB_TEXT = 2,
    CONSOLE_OUTPUT_CELL_TEXT = 3
} console_output_mode_t;
