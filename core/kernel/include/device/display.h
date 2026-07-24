#ifndef BHARAT_DEVICE_DISPLAY_H
#define BHARAT_DEVICE_DISPLAY_H

#include <stdint.h>
#include <stddef.h>

/*
 * Bharat-OS Universal Display Device Interface
 *
 * Provides an abstraction for linear framebuffers independent of UI toolkits.
 * UI libraries like `fbui` bind to this layer.
 */

// Common pixel formats
typedef enum {
    BHARAT_PIXEL_FORMAT_UNKNOWN = 0,
    BHARAT_PIXEL_FORMAT_RGB565,
    BHARAT_PIXEL_FORMAT_ARGB8888,
    BHARAT_PIXEL_FORMAT_XRGB8888,
    BHARAT_PIXEL_FORMAT_ABGR8888
} bharat_pixel_format_t;

// Framebuffer resolution and layout details
typedef struct {
    uint32_t width;        // Horizontal pixels
    uint32_t height;       // Vertical pixels
    uint32_t stride;       // Bytes per row (pitch)
    bharat_pixel_format_t format;
    uint32_t bpp;          // Bits per pixel
    uint32_t refresh_rate; // In Hz
} bharat_display_mode_t;

// An initialized Framebuffer / Display Device Registration
typedef struct bharat_display_device {
    const char* name;                  // E.g. "EFI GOP", "SHAKTI FB"
    void* framebuffer_base;            // MMIO physical base address (to be mapped)
    void* framebuffer_vaddr;           // Kernel/User mapped virtual address
    uint64_t framebuffer_size;         // Total byte size of the frame buffer

    bharat_display_mode_t current_mode;

    // Core driver callbacks
    int (*set_mode)(struct bharat_display_device* dev, const bharat_display_mode_t* mode);
    void (*update_damage)(struct bharat_display_device* dev, uint32_t x, uint32_t y, uint32_t w, uint32_t h);
    void (*blank)(struct bharat_display_device* dev, int enable);

    // Linked list hook for the display manager
    struct bharat_display_device* next;
} bharat_display_device_t;

// Globals to manage displays at the core kernel layer
int display_register(bharat_display_device_t* dev);
bharat_display_device_t* display_get_primary(void);

#endif /* BHARAT_DEVICE_DISPLAY_H */
