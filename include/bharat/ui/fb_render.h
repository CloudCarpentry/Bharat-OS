#ifndef BHARAT_FB_RENDER_H
#define BHARAT_FB_RENDER_H

#include "bharat/display/display.h"

/**
 * Basic 2D Rendering Primitives over a raw framebuffer
 */
typedef struct {
    bharat_display_device_t *device;
    uint32_t foreground_color;
    uint32_t background_color;
} fbui_render_context_t;

void fbui_render_init(fbui_render_context_t *ctx, bharat_display_device_t *dev);
void fbui_render_fill_rect(fbui_render_context_t *ctx, uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color);
void fbui_render_draw_line(fbui_render_context_t *ctx, uint32_t x0, uint32_t y0, uint32_t x1, uint32_t y1, uint32_t color);
void fbui_render_blit(fbui_render_context_t *ctx, const void *src, uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t src_stride);

#endif // BHARAT_FB_RENDER_H
