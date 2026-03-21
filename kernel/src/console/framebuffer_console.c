#include "console/console_backend.h"
#include "console/console_render.h"
#include <stddef.h>

static bool fb_init(console_backend_t *backend) {
    if (!backend || !backend->state) return false;
    framebuffer_console_state_t *state = (framebuffer_console_state_t *)backend->state;
    console_render_fb_init(state);
    return true;
}

static size_t fb_write(console_backend_t *backend, const char *data, size_t len) {
    if (!backend || !backend->state || !data) return 0;
    framebuffer_console_state_t *state = (framebuffer_console_state_t *)backend->state;

    for (size_t i = 0; i < len; i++) {
        console_render_fb_write_char(state, data[i]);
    }
    return len;
}

static void fb_flush(console_backend_t *backend) {
    (void)backend; // Typically memory mapped, no explicit flush needed unless cached
}

static console_caps_t fb_query_caps(console_backend_t *backend) {
    (void)backend;
    return CON_CAP_VISIBLE_SINK |
           CON_CAP_RUNTIME |
           CON_CAP_COLOR |
           CON_CAP_CURSOR |
           CON_CAP_CLEAR |
           CON_CAP_SCROLL |
           CON_CAP_GEOMETRY |
           CON_CAP_FRAMEBUFFER_TEXT;
}

static bool fb_clear(console_backend_t *backend) {
    if (!backend || !backend->state) return false;
    framebuffer_console_state_t *state = (framebuffer_console_state_t *)backend->state;
    console_render_fb_clear(state);
    return true;
}

static bool fb_set_cursor(console_backend_t *backend, console_rows_t row, console_cols_t col) {
    if (!backend || !backend->state) return false;
    framebuffer_console_state_t *state = (framebuffer_console_state_t *)backend->state;
    console_render_fb_set_cursor(state, row, col);
    return true;
}

static bool fb_get_geometry(console_backend_t *backend, console_rows_t *rows, console_cols_t *cols) {
    if (!backend || !backend->state || !rows || !cols) return false;
    framebuffer_console_state_t *state = (framebuffer_console_state_t *)backend->state;
    *rows = state->rows;
    *cols = state->cols;
    return true;
}

const console_backend_ops_t framebuffer_console_ops = {
    .init = fb_init,
    .late_init = NULL,
    .shutdown = NULL,
    .write = fb_write,
    .write_atomic = fb_write, // Fast un-locked path for panics
    .flush = fb_flush,
    .panic_flush = fb_flush,
    .query_caps = fb_query_caps,
    .set_mode = NULL,
    .clear = fb_clear,
    .set_cursor = fb_set_cursor,
    .get_geometry = fb_get_geometry,
    .poll_input = NULL
};
