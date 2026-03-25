#include "console/console_render.h"
#include <stddef.h>
#include "arch/memops.h"

/* Extremely basic fixed-width rendering mechanics. */
#define FONT_WIDTH  8
#define FONT_HEIGHT 16

static void draw_pixel(framebuffer_console_state_t *state, uint32_t x, uint32_t y, uint32_t color) {
    if (!state || !state->fb_base) return;
    if (x >= state->width_px || y >= state->height_px) return;

    // Simplified assuming 32bpp
    volatile uint32_t *pixel = (volatile uint32_t *)(state->fb_base + (y * state->stride_bytes) + (x * 4));
    *pixel = color;
}

static void draw_char_glyph(framebuffer_console_state_t *state, char c, console_cols_t col, console_rows_t row) {
    if (!state || !state->font) return;

    // Very simplified standard 8x16 fixed font access
    const uint8_t *glyph = (const uint8_t *)state->font + ((uint8_t)c * FONT_HEIGHT);

    uint32_t start_x = col * FONT_WIDTH;
    uint32_t start_y = row * FONT_HEIGHT;

    for (uint32_t y = 0; y < FONT_HEIGHT; y++) {
        uint8_t row_bits = glyph[y];
        for (uint32_t x = 0; x < FONT_WIDTH; x++) {
            if (row_bits & (1 << (7 - x))) {
                draw_pixel(state, start_x + x, start_y + y, state->fg_color);
            } else {
                draw_pixel(state, start_x + x, start_y + y, state->bg_color);
            }
        }
    }
}

void console_render_fb_init(framebuffer_console_state_t *state) {
    if (!state) return;
    state->rows = state->height_px / FONT_HEIGHT;
    state->cols = state->width_px / FONT_WIDTH;
    state->cursor_row = 0;
    state->cursor_col = 0;
    state->clear_supported = true;
    state->scroll_supported = true;

    console_render_fb_clear(state);
}

void console_render_fb_clear(framebuffer_console_state_t *state) {
    if (!state || !state->fb_base) return;

    uint32_t total_bytes = state->stride_bytes * state->height_px;

    if (state->bg_color == 0) {
        arch_memset((void*)state->fb_base, 0, total_bytes, ARCH_MEMOP_F_DEFAULT);
    } else {
        // We only implement bulk clear for black background efficiently here.
        // For other colors, we fallback to char by char.
        // A future hardware acceleration driver would implement full solid fills.
        for (uint32_t y = 0; y < state->height_px; y++) {
            for (uint32_t x = 0; x < state->width_px; x++) {
                 draw_pixel(state, x, y, state->bg_color);
            }
        }
    }

    state->cursor_row = 0;
    state->cursor_col = 0;
}

void console_render_fb_scroll(framebuffer_console_state_t *state) {
    if (!state || !state->fb_base) return;

    // Move all rows up by one.
    uint32_t row_bytes = state->stride_bytes * FONT_HEIGHT;
    uint32_t scroll_bytes = state->stride_bytes * (state->height_px - FONT_HEIGHT);

    arch_memmove((void*)state->fb_base, (const void*)(state->fb_base + row_bytes), scroll_bytes, ARCH_MEMOP_F_DEFAULT);

    // Clear last row
    uint32_t last_row_start_y = state->height_px - FONT_HEIGHT;
    uint8_t *last_row_ptr = (uint8_t *)state->fb_base + (last_row_start_y * state->stride_bytes);
    uint32_t last_row_bytes = state->stride_bytes * FONT_HEIGHT;

    if (state->bg_color == 0) {
        arch_memset((void*)last_row_ptr, 0, last_row_bytes, ARCH_MEMOP_F_DEFAULT);
    } else {
        for (uint32_t y = last_row_start_y; y < state->height_px; y++) {
            for (uint32_t x = 0; x < state->width_px; x++) {
                 draw_pixel(state, x, y, state->bg_color);
            }
        }
    }
}

void console_render_fb_set_cursor(framebuffer_console_state_t *state, console_rows_t row, console_cols_t col) {
    if (!state) return;
    if (row < state->rows) state->cursor_row = row;
    if (col < state->cols) state->cursor_col = col;
}

void console_render_fb_write_char(framebuffer_console_state_t *state, char c) {
    if (!state) return;

    if (c == '\n') {
        state->cursor_col = 0;
        state->cursor_row++;
    } else if (c == '\r') {
        state->cursor_col = 0;
    } else if (c == '\t') {
        state->cursor_col = (state->cursor_col + 8) & ~7;
        if (state->cursor_col >= state->cols) {
            state->cursor_col = 0;
            state->cursor_row++;
        }
    } else {
        draw_char_glyph(state, c, state->cursor_col, state->cursor_row);
        state->cursor_col++;

        if (state->cursor_col >= state->cols) {
            state->cursor_col = 0;
            state->cursor_row++;
        }
    }

    if (state->cursor_row >= state->rows) {
        console_render_fb_scroll(state);
        state->cursor_row = state->rows - 1;
    }
}

void console_render_fb_write(framebuffer_console_state_t *state, const char *data, size_t len) {
    if (!state || !data) return;

    // Instead of calling _write_char and repeatedly checking wrap/scroll for every char,
    // we do an optimized bulk write.
    for (size_t i = 0; i < len; i++) {
        char c = data[i];

        if (c == '\n') {
            state->cursor_col = 0;
            state->cursor_row++;
        } else if (c == '\r') {
            state->cursor_col = 0;
        } else if (c == '\t') {
            state->cursor_col = (state->cursor_col + 8) & ~7;
            if (state->cursor_col >= state->cols) {
                state->cursor_col = 0;
                state->cursor_row++;
            }
        } else {
            draw_char_glyph(state, c, state->cursor_col, state->cursor_row);
            state->cursor_col++;

            if (state->cursor_col >= state->cols) {
                state->cursor_col = 0;
                state->cursor_row++;
            }
        }

        // Only scroll if we really fell off the edge during this loop
        if (state->cursor_row >= state->rows) {
            console_render_fb_scroll(state);
            state->cursor_row = state->rows - 1;
        }
    }
}
