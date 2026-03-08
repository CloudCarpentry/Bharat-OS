#include "bharat/ui/fb_render.h"
#include <string.h>
#include <stdint.h>
#include <math.h>

/**
 * Initialize a new rendering context for a framebuffer device.
 */
void fbui_render_init(fbui_render_context_t *ctx, bharat_display_device_t *dev) {
    if (!ctx || !dev) return;
    ctx->device = dev;
    ctx->foreground_color = 0xFFFFFFFF; // White
    ctx->background_color = 0xFF000000; // Black
}

/**
 * Basic ARGB8888 fill rectangle directly mapping to VRAM/RAM buffer.
 * Simple loops meant to be overridden by DMA if available later.
 */
void fbui_render_fill_rect(fbui_render_context_t *ctx, uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color) {
    if (!ctx || !ctx->device || !ctx->device->framebuffer_base) return;

    // Fast-path ARGB8888 assumptions
    if (ctx->device->current_mode.format == BHARAT_PIXEL_FORMAT_ARGB8888 ||
        ctx->device->current_mode.format == BHARAT_PIXEL_FORMAT_XRGB8888) {

        uint32_t *fb = (uint32_t *)ctx->device->framebuffer_base;
        uint32_t stride = ctx->device->current_mode.stride / 4; // Stride in words

        for (uint32_t j = y; j < y + h; j++) {
            if (j >= ctx->device->current_mode.height) break;
            for (uint32_t i = x; i < x + w; i++) {
                if (i >= ctx->device->current_mode.width) break;
                fb[j * stride + i] = color;
            }
        }

        // Push to hardware if a flush is needed
        bharat_display_update_damage(ctx->device, x, y, w, h);
    }
}

/**
 * Bresenham's line algorithm.
 * Hardcoded to 32bpp for brevity, should scale based on bpp.
 */
void fbui_render_draw_line(fbui_render_context_t *ctx, uint32_t x0, uint32_t y0, uint32_t x1, uint32_t y1, uint32_t color) {
    if (!ctx || !ctx->device || !ctx->device->framebuffer_base) return;

    uint32_t *fb = (uint32_t *)ctx->device->framebuffer_base;
    uint32_t stride = ctx->device->current_mode.stride / 4;

    int dx = (x1 > x0) ? (x1 - x0) : (x0 - x1);
    int dy = (y1 > y0) ? -(y1 - y0) : (y0 - y1);
    int sx = x0 < x1 ? 1 : -1;
    int sy = y0 < y1 ? 1 : -1;
    int err = dx + dy, e2;

    for (;;) {
        if (x0 < ctx->device->current_mode.width && y0 < ctx->device->current_mode.height) {
            fb[y0 * stride + x0] = color;
        }

        if (x0 == x1 && y0 == y1) break;
        e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }

    // Simplistic damage reporting (bound box would be better)
    uint32_t min_x = (x0 < x1) ? x0 : x1;
    uint32_t min_y = (y0 < y1) ? y0 : y1;
    uint32_t max_x = (x0 > x1) ? x0 : x1;
    uint32_t max_y = (y0 > y1) ? y0 : y1;
    bharat_display_update_damage(ctx->device, min_x, min_y, max_x - min_x + 1, max_y - min_y + 1);
}

/**
 * Basic software blit (copy from memory to framebuffer).
 * For images and sprites.
 */
void fbui_render_blit(fbui_render_context_t *ctx, const void *src, uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t src_stride) {
    if (!ctx || !ctx->device || !ctx->device->framebuffer_base || !src) return;

    uint8_t *fb = (uint8_t *)ctx->device->framebuffer_base;
    uint32_t dst_stride = ctx->device->current_mode.stride;

    const uint8_t *src_pixels = (const uint8_t *)src;

    for (uint32_t j = 0; j < h; j++) {
        uint32_t py = y + j;
        if (py >= ctx->device->current_mode.height) break;

        uint32_t fb_offset = (py * dst_stride) + (x * 4); // Assuming 32bpp
        uint32_t src_offset = j * src_stride;

        // Clip copy width
        uint32_t copy_width = (x + w <= ctx->device->current_mode.width) ? w : (ctx->device->current_mode.width - x);

        memcpy(fb + fb_offset, src_pixels + src_offset, copy_width * 4);
    }

    bharat_display_update_damage(ctx->device, x, y, w, h);
}
