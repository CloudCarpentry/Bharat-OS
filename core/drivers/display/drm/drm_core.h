#ifndef BHARATOS_DRM_CORE_H
#define BHARATOS_DRM_CORE_H

#include "drm_device.h"

int drm_device_register(drm_device_t *dev);
void drm_device_unregister(drm_device_t *dev);

int drm_mode_validate(drm_device_t *dev, const drm_mode_t *mode);
int drm_fb_create(drm_device_t *dev, uint32_t width, uint32_t height, uint32_t format, drm_fb_t **out_fb);
void drm_fb_destroy(drm_fb_t *fb);
int drm_set_scanout(drm_device_t *dev, drm_fb_t *fb);

#endif /* BHARATOS_DRM_CORE_H */