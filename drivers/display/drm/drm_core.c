#include "drm_core.h"
#include <stddef.h>

int drm_mode_validate(drm_device_t *dev, const drm_mode_t *mode) {
    if (!dev || !dev->ops || !mode) return -1;
    if (dev->ops->mode_valid) {
        return dev->ops->mode_valid(dev, mode);
    }
    return 0; // Assume valid if no constraint hook is present
}

int drm_fb_create(drm_device_t *dev, uint32_t width, uint32_t height, uint32_t format, drm_fb_t **out_fb) {
    if (!dev || !dev->ops || !out_fb || width == 0 || height == 0) return -1;
    if (dev->ops->fb_create) {
        return dev->ops->fb_create(dev, width, height, format, out_fb);
    }
    return -1; // Unsupported
}

void drm_fb_destroy(drm_fb_t *fb) {
    if (!fb || !fb->dev || !fb->dev->ops) return;
    if (fb->dev->ops->fb_destroy) {
        fb->dev->ops->fb_destroy(fb->dev, fb);
    }
}

int drm_set_scanout(drm_device_t *dev, drm_fb_t *fb) {
    if (!dev || !dev->ops || !fb) return -1;
    if (fb->dev != dev) return -1; // Must belong to same device

    if (dev->ops->set_scanout) {
        return dev->ops->set_scanout(dev, fb);
    }
    return -1; // Unsupported
}