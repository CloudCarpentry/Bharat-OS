#ifndef BHARAT_UAPI_DISPLAY_H
#define BHARAT_UAPI_DISPLAY_H

#include <stdint.h>
#include "bharat/uapi/device/device_class.h"

/**
 * Display device class specialized for UAPI.
 */
typedef enum {
    BHARAT_DISPLAY_CLASS_FRAMEBUFFER = 1,
    BHARAT_DISPLAY_CLASS_GPU = 2,
    BHARAT_DISPLAY_CLASS_VIRTIO_GPU = 3,
    BHARAT_DISPLAY_CLASS_PANEL = 4,
    BHARAT_DISPLAY_CLASS_HEADLESS = 5,
} bharat_display_class_t;

/**
 * Display mode descriptor.
 */
typedef struct {
    uint32_t width;
    uint32_t height;
    uint32_t refresh_hz;
    uint32_t pixel_format;
    uint32_t flags;
} bharat_display_mode_t;

#endif /* BHARAT_UAPI_DISPLAY_H */
