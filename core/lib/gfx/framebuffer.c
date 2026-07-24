#include "framebuffer.h"

uint32_t bharat_gfx_framebuffer_min_stride(uint32_t width, bharat_pixel_format_t fmt) {
    uint32_t bpp;
    if (fmt == BHARAT_PIXFMT_NV12) {
        /* Stride for NV12 refers to the Y plane (8 bits per pixel) */
        bpp = 8;
    } else {
        bpp = bharat_pixel_format_bpp(fmt);
    }

    if (bpp == 0) return 0;

    /* Ensure 4-byte alignment for rows as a baseline */
    uint32_t stride = (width * bpp + 7) / 8;
    return (stride + 3) & ~3;
}

size_t bharat_gfx_framebuffer_min_size(uint32_t width, uint32_t height, bharat_pixel_format_t fmt) {
    uint32_t stride = bharat_gfx_framebuffer_min_stride(width, fmt);
    if (stride == 0) return 0;

    if (fmt == BHARAT_PIXFMT_NV12) {
        /* NV12 has a Y plane and a packed UV plane (1.5x Y size) */
        return (size_t)stride * height * 3 / 2;
    }

    return (size_t)stride * height;
}

bool bharat_gfx_framebuffer_is_valid(const bharat_framebuffer_t *fb) {
    if (!fb) return false;
    if (fb->width == 0 || fb->height == 0) return false;
    if (fb->pixel_format == BHARAT_PIXFMT_UNKNOWN || fb->pixel_format >= BHARAT_PIXFMT_MAX) return false;

    uint32_t min_stride = bharat_gfx_framebuffer_min_stride(fb->width, fb->pixel_format);
    if (fb->stride_bytes < min_stride) return false;

    return true;
}
