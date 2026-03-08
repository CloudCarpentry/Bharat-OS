#include "bharat/ui/fb_console.h"
#include "bharat/ui/fb_render.h"

// Font properties (dummy 8x16 font assumptions)
#define FONT_WIDTH  8
#define FONT_HEIGHT 16

static struct {
    fbui_render_context_t render_ctx;
    uint32_t cursor_x;
    uint32_t cursor_y;
    uint32_t cols;
    uint32_t rows;
    bool initialized;
} fb_console_state = {0};

/**
 * Initialize a raw framebuffer console for logging.
 */
void fb_console_init(bharat_display_device_t *dev) {
    if (!dev) return;

    fbui_render_init(&fb_console_state.render_ctx, dev);

    // Default colors for console
    fb_console_state.render_ctx.background_color = 0xFF000000; // Black
    fb_console_state.render_ctx.foreground_color = 0xFFFFFFFF; // White

    fb_console_state.cols = dev->current_mode.width / FONT_WIDTH;
    fb_console_state.rows = dev->current_mode.height / FONT_HEIGHT;
    fb_console_state.cursor_x = 0;
    fb_console_state.cursor_y = 0;

    fb_console_state.initialized = true;

    // Clear screen
    fb_console_clear();
}

/**
 * Clear the console area.
 */
void fb_console_clear(void) {
    if (!fb_console_state.initialized) return;

    bharat_display_device_t *dev = fb_console_state.render_ctx.device;
    fbui_render_fill_rect(&fb_console_state.render_ctx, 0, 0, dev->current_mode.width, dev->current_mode.height, fb_console_state.render_ctx.background_color);

    fb_console_state.cursor_x = 0;
    fb_console_state.cursor_y = 0;
}

/**
 * Output a single character.
 */
void fb_console_putc(char c) {
    if (!fb_console_state.initialized) return;

    if (c == '\n') {
        fb_console_state.cursor_x = 0;
        fb_console_state.cursor_y++;
    } else if (c == '\r') {
        fb_console_state.cursor_x = 0;
    } else if (c == '\t') {
        fb_console_state.cursor_x = (fb_console_state.cursor_x + 8) & ~7;
    } else {
        // Here we would lookup the actual glyph from a font table.
        // For the sake of the skeleton, we draw a colored box.
        uint32_t px = fb_console_state.cursor_x * FONT_WIDTH;
        uint32_t py = fb_console_state.cursor_y * FONT_HEIGHT;

        // Draw the "character" (a filled rectangle for now until fonts are loaded)
        fbui_render_fill_rect(&fb_console_state.render_ctx, px, py, FONT_WIDTH, FONT_HEIGHT, fb_console_state.render_ctx.foreground_color);

        fb_console_state.cursor_x++;
    }

    // Scroll check
    if (fb_console_state.cursor_x >= fb_console_state.cols) {
        fb_console_state.cursor_x = 0;
        fb_console_state.cursor_y++;
    }

    if (fb_console_state.cursor_y >= fb_console_state.rows) {
        // Normally memmove the framebuffer up and clear the last line.
        // For simplicity: loop back to top.
        fb_console_state.cursor_y = 0;
        fb_console_clear();
    }
}

/**
 * Output a string.
 */
void fb_console_puts(const char *str) {
    if (!str || !fb_console_state.initialized) return;

    while (*str) {
        fb_console_putc(*str++);
    }
}
