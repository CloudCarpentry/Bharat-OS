#include "bharat/display/display.h"
#include <string.h>

#define BHARAT_MAX_DISPLAYS 4

static bharat_display_device_t *g_displays[BHARAT_MAX_DISPLAYS];
static uint32_t g_num_displays = 0;

/**
 * Register a display device into the Bharat-OS subsystem.
 */
int bharat_display_register(bharat_display_device_t *dev) {
    if (!dev || !dev->ops) {
        return -1; // Invalid device
    }

    if (g_num_displays >= BHARAT_MAX_DISPLAYS) {
        return -2; // Too many displays
    }

    dev->id = g_num_displays;
    g_displays[g_num_displays++] = dev;

    // Default enablement on registration (can be deferred by policy later)
    if (dev->ops->enable) {
        dev->ops->enable(dev);
    }

    return 0; // Success
}

/**
 * Retrieves the primary default display device.
 */
bharat_display_device_t* bharat_display_get_default(void) {
    if (g_num_displays == 0) {
        return NULL;
    }
    return g_displays[0];
}

/**
 * Submit damage/dirty rectangles to trigger a flush on panels
 * requiring explicit SPI/DSI updates. Returns 0 on success.
 */
int bharat_display_update_damage(bharat_display_device_t *dev, uint32_t x, uint32_t y, uint32_t w, uint32_t h) {
    if (!dev || !dev->ops) return -1;

    // Optional bounds checking could be added here
    if (x >= dev->current_mode.width || y >= dev->current_mode.height) {
        return -1; // Out of bounds
    }

    // Clip width and height
    if (x + w > dev->current_mode.width) {
        w = dev->current_mode.width - x;
    }
    if (y + h > dev->current_mode.height) {
        h = dev->current_mode.height - y;
    }

    // Only flush if the hardware requires it (e.g., SimpleDRM, SPI panels)
    if (dev->requires_flush && dev->ops->flush) {
        return dev->ops->flush(dev, x, y, w, h);
    }

    return 0; // Memory-mapped framebuffers might not require explicit flushes.
}
