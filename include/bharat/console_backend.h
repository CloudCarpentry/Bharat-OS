#ifndef BHARAT_CONSOLE_BACKEND_H
#define BHARAT_CONSOLE_BACKEND_H

#include "console_types.h"
#include "console_caps.h"
#include <stddef.h>

struct console_backend;

/*
 * Backend operations table
 */
typedef struct console_backend_ops {
    int (*init)(struct console_backend* backend);
    int (*late_init)(struct console_backend* backend);
    void (*write)(struct console_backend* backend, const char* str, size_t len);
    void (*write_atomic)(struct console_backend* backend, const char* str, size_t len);
    void (*flush)(struct console_backend* backend);
    void (*panic_flush)(struct console_backend* backend);
    int (*set_mode)(struct console_backend* backend, uint32_t mode);
    uint64_t (*query_caps)(struct console_backend* backend);
    void (*get_geometry)(struct console_backend* backend, uint32_t* cols, uint32_t* rows);
    void (*scroll)(struct console_backend* backend, int lines);
    void (*clear)(struct console_backend* backend);
    void (*set_palette)(struct console_backend* backend, uint32_t* palette, size_t count);
    int (*poll_input)(struct console_backend* backend, char* out_char);
} console_backend_ops_t;

/*
 * Registered console backend
 */
typedef struct console_backend {
    console_backend_type_t type;
    const char* name;
    uint64_t caps;

    // Policy / Routing configuration
    bool enabled;
    console_log_level_t min_level; // Minimum severity this backend logs
    uint32_t priority;

    bool early_ok;
    bool panic_ok;

    const console_backend_ops_t* ops;
    void* priv_data;

    struct console_backend* next;
} console_backend_t;

#endif // BHARAT_CONSOLE_BACKEND_H
