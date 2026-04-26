#ifndef BHARAT_LIB_GFX_PIXEL_FORMAT_H
#define BHARAT_LIB_GFX_PIXEL_FORMAT_H

#include <stdint.h>

/**
 * Standard Bharat-OS Pixel Formats
 * Aligned with common DRM/FourCC definitions where possible.
 */
typedef enum {
    BHARAT_PIXFMT_UNKNOWN = 0,

    /* 32-bit RGB */
    BHARAT_PIXFMT_XRGB8888,
    BHARAT_PIXFMT_ARGB8888,
    BHARAT_PIXFMT_BGRX8888,
    BHARAT_PIXFMT_BGRA8888,

    /* 16-bit RGB */
    BHARAT_PIXFMT_RGB565,
    BHARAT_PIXFMT_BGR565,

    /* YUV */
    BHARAT_PIXFMT_NV12,
    BHARAT_PIXFMT_YUYV,

    BHARAT_PIXFMT_MAX
} bharat_pixel_format_t;

/**
 * Returns the bits per pixel for a given format.
 */
static inline uint32_t bharat_pixel_format_bpp(bharat_pixel_format_t fmt) {
    switch (fmt) {
        case BHARAT_PIXFMT_XRGB8888:
        case BHARAT_PIXFMT_ARGB8888:
        case BHARAT_PIXFMT_BGRX8888:
        case BHARAT_PIXFMT_BGRA8888:
            return 32;
        case BHARAT_PIXFMT_RGB565:
        case BHARAT_PIXFMT_BGR565:
            return 16;
        case BHARAT_PIXFMT_NV12:
            return 12; /* Average bpp */
        case BHARAT_PIXFMT_YUYV:
            return 16;
        default:
            return 0;
    }
}

#endif /* BHARAT_LIB_GFX_PIXEL_FORMAT_H */
