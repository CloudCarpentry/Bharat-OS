#pragma once

#include "console_base_types.h"
#include <stdbool.h>

typedef struct {
    volatile uint8_t *fb_base;
    uint32_t width_px;
    uint32_t height_px;
    uint32_t stride_bytes;
    uint32_t pixel_format;
    console_rows_t rows;
    console_cols_t cols;
    console_rows_t cursor_row;
    console_cols_t cursor_col;
    uint32_t fg_color;
    uint32_t bg_color;
    const void *font;
    bool clear_supported;
    bool scroll_supported;
} framebuffer_console_state_t;

void console_render_fb_init(framebuffer_console_state_t *state);
void console_render_fb_write_char(framebuffer_console_state_t *state, char c);
void console_render_fb_write(framebuffer_console_state_t *state, const char *data, size_t len);
void console_render_fb_clear(framebuffer_console_state_t *state);
void console_render_fb_scroll(framebuffer_console_state_t *state);
void console_render_fb_set_cursor(framebuffer_console_state_t *state, console_rows_t row, console_cols_t col);
