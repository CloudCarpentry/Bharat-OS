#pragma once

#include "console_base_types.h"
#include "console_types.h"
#include "console_caps.h"
#include "console_record.h"

struct console_backend;

typedef struct console_backend_ops {
    bool (*init)(struct console_backend *backend);
    bool (*late_init)(struct console_backend *backend);
    void (*shutdown)(struct console_backend *backend);

    size_t (*write)(struct console_backend *backend, const char *data, size_t len);
    size_t (*write_atomic)(struct console_backend *backend, const char *data, size_t len);

    void (*flush)(struct console_backend *backend);
    void (*panic_flush)(struct console_backend *backend);

    console_caps_t (*query_caps)(struct console_backend *backend);

    bool (*set_mode)(struct console_backend *backend, console_output_mode_t mode);
    bool (*clear)(struct console_backend *backend);
    bool (*set_cursor)(struct console_backend *backend, console_rows_t row, console_cols_t col);
    bool (*get_geometry)(struct console_backend *backend, console_rows_t *rows, console_cols_t *cols);

    int (*poll_input)(struct console_backend *backend);
} console_backend_ops_t;

typedef struct console_backend {
    const char *name;
    console_backend_type_t type;
    const console_backend_ops_t *ops;
    console_caps_t caps;
    console_level_t min_level;
    uint32_t priority;
    bool enabled;
    bool early_ok;
    bool panic_ok;
    void *state;
} console_backend_t;
