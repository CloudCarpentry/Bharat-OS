#ifndef BHARATOS_DRM_MODE_H
#define BHARATOS_DRM_MODE_H

#include <stdint.h>

typedef struct drm_mode {
    uint32_t width;
    uint32_t height;
    uint32_t refresh_rate;
    uint32_t format;
} drm_mode_t;

#endif /* BHARATOS_DRM_MODE_H */