#ifndef BHARATOS_DRM_REGISTRY_H
#define BHARATOS_DRM_REGISTRY_H

#include "drm_core.h"

void drm_registry_init(void);

drm_device_t *drm_get_device(int id);

#endif /* BHARATOS_DRM_REGISTRY_H */