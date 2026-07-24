#include "security/vfio.h"
#include <stddef.h>

#define BHARAT_MAX_VFIO_CONTAINERS 16U

typedef struct {
    uint8_t used;
    bharat_vfio_container_t container;
} bharat_vfio_container_slot_t;

static bharat_vfio_container_slot_t g_vfio_containers[BHARAT_MAX_VFIO_CONTAINERS];

int bharat_vfio_container_create(bharat_vfio_container_t** out_container) {
    uint32_t i;
    if (!out_container) {
        return -1;
    }

    for (i = 0; i < BHARAT_MAX_VFIO_CONTAINERS; ++i) {
        if (!g_vfio_containers[i].used) {
            g_vfio_containers[i].used = 1U;
            g_vfio_containers[i].container.container_id = i;
            g_vfio_containers[i].container.flags = BHARAT_VFIO_CONTAINER_FLAG_ACTIVE;
            g_vfio_containers[i].container.domain = NULL;
            g_vfio_containers[i].container.group_refcount = 0;
            *out_container = &g_vfio_containers[i].container;
            return 0;
        }
    }

    return -1;
}

int bharat_vfio_container_destroy(bharat_vfio_container_t* container) {
    uint32_t i;
    if (!container) {
        return -1;
    }

    if (container->group_refcount > 0) {
        return -1; // Cannot destroy while groups are attached
    }

    for (i = 0; i < BHARAT_MAX_VFIO_CONTAINERS; ++i) {
        if (g_vfio_containers[i].used && &g_vfio_containers[i].container == container) {
            if (container->domain) {
                bharat_iommu_domain_destroy(container->domain);
                container->domain = NULL;
            }
            g_vfio_containers[i].used = 0U;
            g_vfio_containers[i].container.flags = 0U;
            return 0;
        }
    }

    return -1;
}

int bharat_vfio_group_attach_container(bharat_vfio_group_t* group, bharat_vfio_container_t* container) {
    if (!group || !container) {
        return -1;
    }

    /* Verify container is active */
    if (!(container->flags & BHARAT_VFIO_CONTAINER_FLAG_ACTIVE)) {
        return -1;
    }

    /* Create IOMMU domain if not already created for this container */
    if (!container->domain) {
        bharat_iommu_domain_config_t config = {
            .type = BHARAT_IOMMU_DOMAIN_USER,
            .flags = 0,
        };
        int ret = bharat_iommu_domain_create(&config, &container->domain);
        if (ret < 0) {
            return ret;
        }
    }

    /* Attach group to domain */
    int ret = bharat_iommu_group_attach(group->iommu_group, container->domain);
    if (ret < 0) {
        /* If this was the first group and attach failed, cleanup domain */
        if (container->group_refcount == 0) {
            bharat_iommu_domain_destroy(container->domain);
            container->domain = NULL;
        }
        return ret;
    }

    group->domain = container->domain;
    container->group_refcount++;
    return 0;
}

int bharat_vfio_group_detach_container(bharat_vfio_group_t* group) {
    if (!group) {
        return -1;
    }

    if (!group->domain) {
        return -1;
    }

    bharat_iommu_domain_t *domain = group->domain;

    int ret = bharat_iommu_group_detach(group->iommu_group);
    if (ret < 0) {
        return ret;
    }

    group->domain = NULL;

    /* Find the container that owns this domain */
    bharat_vfio_container_t* container = NULL;
    for (uint32_t i = 0; i < BHARAT_MAX_VFIO_CONTAINERS; ++i) {
        if (g_vfio_containers[i].used && g_vfio_containers[i].container.domain == domain) {
            container = &g_vfio_containers[i].container;
            break;
        }
    }

    if (container) {
        if (container->group_refcount > 0) {
            container->group_refcount--;

            if (container->group_refcount == 0) {
                bharat_iommu_domain_destroy(container->domain);
                container->domain = NULL;
            }
        }
    }

    return 0;
}

int bharat_vfio_device_bind(bharat_vfio_device_t* dev, bharat_vfio_group_t* group) {
    if (!dev || !group) {
        return -1;
    }

    /* Prevent binding a device that is already bound */
    if (dev->flags & BHARAT_VFIO_DEVICE_FLAG_BOUND) {
        return -1;
    }

    dev->group = group;
    dev->flags |= BHARAT_VFIO_DEVICE_FLAG_BOUND;
    return 0;
}

int bharat_vfio_device_unbind(bharat_vfio_device_t* dev) {
    if (!dev) {
        return -1;
    }

    dev->group = NULL;
    dev->flags &= ~BHARAT_VFIO_DEVICE_FLAG_BOUND;
    return 0;
}
