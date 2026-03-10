#ifndef BHARAT_HAL_IOMMU_H
#define BHARAT_HAL_IOMMU_H

#include <stddef.h>
#include <stdint.h>
#include "security/isolation.h"

typedef struct hal_iommu_domain hal_iommu_domain_t;
typedef struct hal_iommu_group hal_iommu_group_t;

int hal_iommu_init(void);

int hal_iommu_domain_create(const bharat_iommu_domain_config_t *cfg,
                            hal_iommu_domain_t **out);

int hal_iommu_domain_destroy(hal_iommu_domain_t *domain);

int hal_iommu_group_attach(hal_iommu_group_t *group, hal_iommu_domain_t *domain);

int hal_iommu_group_detach(hal_iommu_group_t *group);

int hal_iommu_map(hal_iommu_domain_t *domain,
                  uint64_t iova, uint64_t phys, size_t size, uint64_t prot);

int hal_iommu_unmap(hal_iommu_domain_t *domain,
                    uint64_t iova, size_t size);

int hal_iommu_block_device(bharat_device_t *dev);

int hal_iommu_get_group(bharat_device_t *dev, hal_iommu_group_t **out_group);

#endif // BHARAT_HAL_IOMMU_H