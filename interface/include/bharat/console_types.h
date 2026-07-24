#ifndef BHARAT_CONSOLE_TYPES_H
#define BHARAT_CONSOLE_TYPES_H

#include <stdint.h>
#include <stdbool.h>

/*
 * Core Console Types
 */

// Backend category types
typedef enum {
    CONSOLE_TYPE_SERIAL = 1,
    CONSOLE_TYPE_FRAMEBUFFER,
    CONSOLE_TYPE_FIRMWARE,
    CONSOLE_TYPE_MEMORY_LOG,
    CONSOLE_TYPE_VGA_TEXT,
    CONSOLE_TYPE_NETWORK
} console_backend_type_t;

// Console log severities
typedef enum {
    CONSOLE_LEVEL_DEBUG = 0,
    CONSOLE_LEVEL_INFO,
    CONSOLE_LEVEL_WARN,
    CONSOLE_LEVEL_ERROR,
    CONSOLE_LEVEL_PANIC
} console_log_level_t;

// Console operating phase (Early -> Runtime -> Panic)
typedef enum {
    CONSOLE_PHASE_EARLY = 0,
    CONSOLE_PHASE_RUNTIME,
    CONSOLE_PHASE_PANIC
} console_phase_t;

#endif // BHARAT_CONSOLE_TYPES_H
