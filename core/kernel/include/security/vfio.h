#ifndef BHARAT_VFIO_H
#define BHARAT_VFIO_H

#include <stdint.h>
#include "security/isolation.h"

/*
 * Bharat-OS VFIO-like Device Assignment Subsystem
 *
 * Provides controlled user-space access to MMIO, IRQ, and DMA mapping
 * without exposing raw physical memory or unrestricted DMA privileges.
 */

#define BHARAT_VFIO_CONTAINER_FLAG_ACTIVE (1U << 0)
#define BHARAT_VFIO_GROUP_FLAG_ISOLATED   (1U << 0)
#define BHARAT_VFIO_DEVICE_FLAG_BOUND     (1U << 0)

typedef struct {
    uint64_t container_id;
    uint32_t flags;
    bharat_iommu_domain_t* domain;
    uint32_t group_refcount;
} bharat_vfio_container_t;

typedef struct {
    uint32_t group_id;
    uint32_t flags;
    bharat_iommu_domain_t* domain;
    bharat_iommu_group_t* iommu_group;
} bharat_vfio_group_t;

typedef struct {
    uint32_t device_id;
    uint32_t vendor_id;
    uint32_t flags;
    bharat_vfio_group_t* group;
} bharat_vfio_device_t;

typedef struct {
    uint64_t iova;
    uint64_t size;
    uint64_t flags;
} bharat_vfio_iova_region_t;

int bharat_vfio_container_create(bharat_vfio_container_t** out_container);
int bharat_vfio_container_destroy(bharat_vfio_container_t* container);

int bharat_vfio_group_attach_container(bharat_vfio_group_t* group, bharat_vfio_container_t* container);
int bharat_vfio_group_detach_container(bharat_vfio_group_t* group);

int bharat_vfio_device_bind(bharat_vfio_device_t* dev, bharat_vfio_group_t* group);
int bharat_vfio_device_unbind(bharat_vfio_device_t* dev);

#endif // BHARAT_VFIO_H
