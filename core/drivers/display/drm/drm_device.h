#ifndef BHARATOS_DRM_DEVICE_H
#define BHARATOS_DRM_DEVICE_H

#include "drm_mode.h"
#include "drm_fb.h"
#include <stdint.h>

struct drm_device;

typedef struct drm_device_ops {
    int (*load)(struct drm_device *dev);
    void (*unload)(struct drm_device *dev);
    int (*mode_valid)(struct drm_device *dev, const drm_mode_t *mode);
    int (*fb_create)(struct drm_device *dev, uint32_t width, uint32_t height, uint32_t format, drm_fb_t **out_fb);
    void (*fb_destroy)(struct drm_device *dev, drm_fb_t *fb);
    int (*set_scanout)(struct drm_device *dev, drm_fb_t *fb);
} drm_device_ops_t;

typedef struct drm_device {
    int id;
    char name[32];
    const drm_device_ops_t *ops;
    void *driver_data;
    int primary_mode; // Just a placeholder for actual device state tracking
} drm_device_t;

#endif /* BHARATOS_DRM_DEVICE_H */