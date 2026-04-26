#ifndef BHARAT_LIB_GFX_FRAMEBUFFER_H
#define BHARAT_LIB_GFX_FRAMEBUFFER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "pixel_format.h"

/**
 * Headless-safe framebuffer abstraction.
 * This is a pure descriptor and validation library.
 */
typedef struct {
    uint32_t width;
    uint32_t height;
    uint32_t stride_bytes;
    bharat_pixel_format_t pixel_format;
    void *pixels;
    uint64_t buffer_id;
} bharat_framebuffer_t;

/**
 * Validates framebuffer parameters.
 * Returns true if the parameters are sane.
 */
bool bharat_gfx_framebuffer_is_valid(const bharat_framebuffer_t *fb);

/**
 * Calculates the minimum stride required for a given width and pixel format.
 */
uint32_t bharat_gfx_framebuffer_min_stride(uint32_t width, bharat_pixel_format_t fmt);

/**
 * Calculates the minimum buffer size required in bytes.
 */
size_t bharat_gfx_framebuffer_min_size(uint32_t width, uint32_t height, bharat_pixel_format_t fmt);

#endif /* BHARAT_LIB_GFX_FRAMEBUFFER_H */
