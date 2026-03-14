#ifndef BHARAT_HAL_HAL_IOMMU_H
#define BHARAT_HAL_HAL_IOMMU_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint64_t domain_id;
    uint32_t flags;
} hal_iommu_domain_desc_t;

int hal_iommu_domain_create(const hal_iommu_domain_desc_t *desc, void **out_domain);
int hal_iommu_domain_destroy(void *domain);

int hal_iommu_attach_device(void *domain, uint64_t device_id);
int hal_iommu_detach_device(void *domain, uint64_t device_id);

int hal_iommu_map_iova(void *domain,
                       uint64_t iova,
                       uint64_t phys,
                       size_t length,
                       uint64_t prot_flags);

int hal_iommu_unmap_iova(void *domain,
                         uint64_t iova,
                         size_t length);

#ifdef __cplusplus
}
#endif

#endif // BHARAT_HAL_HAL_IOMMU_H
