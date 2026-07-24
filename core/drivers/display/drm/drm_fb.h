#ifndef BHARATOS_DRM_FB_H
#define BHARATOS_DRM_FB_H

#include <stdint.h>

struct drm_device;

typedef struct drm_fb {
    struct drm_device *dev;
    uint32_t width;
    uint32_t height;
    uint32_t pitch;
    uint32_t format;
    uint32_t handle;
    void *buffer_vaddr;
    uint64_t buffer_paddr;
} drm_fb_t;

#endif /* BHARATOS_DRM_FB_H */