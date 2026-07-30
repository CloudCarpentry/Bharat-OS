/*
 * boot_display_bridge.c — Bridge between early boot video handoff and generic display registry
 *
 * Implements DEMO-P2-001 by mapping the validated, mapped boot framebuffer
 * to a generic display device registered into the Bharat-OS display subsystem.
 */

#include "bharat/display/display.h"
#include "bharat/display/boot_video.h"
#include "bharat/display/boot_ui_types.h"
#include "hal/hal.h"

// Prevent compiler warning about unused parameters
#ifndef UNUSED
#define UNUSED(x) (void)(x)
#endif

static bharat_display_device_t g_boot_display;
static bool g_boot_display_registered = false;

static int boot_display_enable(struct bharat_display_device *dev) {
    UNUSED(dev);
    return 0; // Success
}

static int boot_display_disable(struct bharat_display_device *dev) {
    UNUSED(dev);
    return 0; // Success
}

static int boot_display_get_mode(struct bharat_display_device *dev, bharat_display_mode_t *mode) {
    if (!dev || !mode) return -1;
    *mode = dev->current_mode;
    return 0;
}

static int boot_display_flush(struct bharat_display_device *dev, uint32_t x, uint32_t y, uint32_t w, uint32_t h) {
    UNUSED(dev);
    UNUSED(x);
    UNUSED(y);
    UNUSED(w);
    UNUSED(h);
    return 0; // Directly memory-mapped framebuffers require no explicit flushing
}

static const bharat_display_device_ops_t g_boot_display_ops = {
    .enable = boot_display_enable,
    .disable = boot_display_disable,
    .set_mode = NULL,
    .get_mode = boot_display_get_mode,
    .flush = boot_display_flush,
    .set_backlight = NULL,
    .mmap = NULL
};

static bool boot_format_to_display_format(pixel_format_t boot_format, bharat_pixel_format_t *out) {
    if (!out) return false;

    switch (boot_format) {
        case PIXEL_FORMAT_ARGB8888:
            *out = BHARAT_PIXEL_FORMAT_ARGB8888;
            return true;
        case PIXEL_FORMAT_XRGB8888:
            *out = BHARAT_PIXEL_FORMAT_XRGB8888;
            return true;
        case PIXEL_FORMAT_RGB565:
            *out = BHARAT_PIXEL_FORMAT_RGB565;
            return true;
        default:
            return false; // Unsupported or BGRX formats fall back safely to prevent red/blue swap
    }
}

int boot_display_register_from_handoff(const boot_video_handoff_t *handoff) {
    if (!handoff || !handoff->valid) {
        hal_serial_write("  [DISPLAY] Bridge: Invalid boot video handoff.\n");
        return -1;
    }

    // Idempotency check
    if (g_boot_display_registered) {
        return 0;
    }

    // Validation checks with overflow-safe arithmetic
    uint32_t w = handoff->width;
    uint32_t h = handoff->height;
    uint32_t stride = handoff->stride_bytes;
    uint64_t size = handoff->size;

    if (w == 0 || h == 0 || stride == 0 || size == 0 || handoff->virt_addr == 0) {
        hal_serial_write("  [DISPLAY] Bridge: Framebuffer geometry zero or virtual address null.\n");
        return -2;
    }

    // Overflow-safe check: height * stride <= size
    uint32_t expected_size;
    if (__builtin_mul_overflow(h, stride, &expected_size)) {
        hal_serial_write("  [DISPLAY] Bridge: Framebuffer size overflow.\n");
        return -3;
    }

    if ((uint64_t)expected_size > size) {
        hal_serial_write("  [DISPLAY] Bridge: Framebuffer declared size is smaller than geometry requirements.\n");
        return -4;
    }

    bharat_pixel_format_t generic_format;
    if (!boot_format_to_display_format(handoff->format, &generic_format)) {
        hal_serial_write("  [DISPLAY] Bridge: Unsupported pixel format; GUI showcase skipped to avoid memory corruption.\n");
        return -5;
    }

    uint32_t bpp = 0;
    if (generic_format == BHARAT_PIXEL_FORMAT_ARGB8888 || generic_format == BHARAT_PIXEL_FORMAT_XRGB8888) {
        bpp = 32;
    } else if (generic_format == BHARAT_PIXEL_FORMAT_RGB565) {
        bpp = 16;
    }

    // Populate the static display structure truthfully
    g_boot_display.name = "Boot Framebuffer Display";
    g_boot_display.priv_data = NULL;
    g_boot_display.framebuffer_base = (void *)(uintptr_t)handoff->virt_addr;
    g_boot_display.framebuffer_size = (size_t)size;

    g_boot_display.current_mode.width = (uint32_t)w;
    g_boot_display.current_mode.height = (uint32_t)h;
    g_boot_display.current_mode.stride = (uint32_t)stride;
    g_boot_display.current_mode.bpp = bpp;
    g_boot_display.current_mode.format = generic_format;
    g_boot_display.current_mode.refresh_rate = 60;

    g_boot_display.ops = &g_boot_display_ops;
    g_boot_display.can_double_buffer = false;
    g_boot_display.has_hardware_cursor = false;
    g_boot_display.requires_flush = false;

    int ret = bharat_display_register(&g_boot_display);
    if (ret == 0) {
        g_boot_display_registered = true;
        hal_serial_write("  [DISPLAY] Boot framebuffer registered successfully.\n");
    } else {
        hal_serial_write("  [DISPLAY] Failed to register display device.\n");
    }

    return ret;
}
