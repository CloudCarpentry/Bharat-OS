#ifndef BHARAT_DISPLAY_H
#define BHARAT_DISPLAY_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/**
 * Pixel Formats (Inspired by DRM formats)
 */
typedef enum {
    BHARAT_PIXEL_FORMAT_UNKNOWN = 0,
    BHARAT_PIXEL_FORMAT_RGB565,
    BHARAT_PIXEL_FORMAT_ARGB8888,
    BHARAT_PIXEL_FORMAT_XRGB8888,
    BHARAT_PIXEL_FORMAT_MONO_8,
} bharat_pixel_format_t;

/**
 * Framebuffer Metadata & Modes
 */
typedef struct {
    uint32_t width;
    uint32_t height;
    uint32_t stride;       // Bytes per line
    uint32_t bpp;          // Bits per pixel
    bharat_pixel_format_t format;
    uint32_t refresh_rate;
} bharat_display_mode_t;

struct bharat_display_device;

/**
 * Operations for a display device.
 * Modeled to allow porting of Linux fbdev/simpledrm drivers.
 */
typedef struct {
    int (*enable)(struct bharat_display_device *dev);
    int (*disable)(struct bharat_display_device *dev);
    int (*set_mode)(struct bharat_display_device *dev, const bharat_display_mode_t *mode);
    int (*get_mode)(struct bharat_display_device *dev, bharat_display_mode_t *mode);
    int (*flush)(struct bharat_display_device *dev, uint32_t x, uint32_t y, uint32_t w, uint32_t h);
    int (*set_backlight)(struct bharat_display_device *dev, uint8_t level);
    void* (*mmap)(struct bharat_display_device *dev);
} bharat_display_device_ops_t;

/**
 * Display Device Abstraction
 */
typedef struct bharat_display_device {
    const char *name;
    uint32_t id;
    void *priv_data;          // Driver private data
    void *framebuffer_base;   // Mapped VADDR of the framebuffer
    size_t framebuffer_size;
    bharat_display_mode_t current_mode;
    const bharat_display_device_ops_t *ops;

    // Capability flags
    bool can_double_buffer;
    bool has_hardware_cursor;
    bool requires_flush; // e.g., SPI panels vs memory-mapped scanout
} bharat_display_device_t;

// Core API
int bharat_display_register(bharat_display_device_t *dev);
bharat_display_device_t* bharat_display_get_default(void);
int bharat_display_update_damage(bharat_display_device_t *dev, uint32_t x, uint32_t y, uint32_t w, uint32_t h);

#endif // BHARAT_DISPLAY_H
