#include "hal/iommu.h"

int hal_iommu_init(void) {
    return -1; // -ENOSYS
}

int hal_iommu_domain_create(const bharat_iommu_domain_config_t *cfg,
                            hal_iommu_domain_t **out) {
    (void)cfg;
    (void)out;
    return -1; // -ENOSYS
}

int hal_iommu_domain_destroy(hal_iommu_domain_t *domain) {
    (void)domain;
    return -1; // -ENOSYS
}

int hal_iommu_group_attach(hal_iommu_group_t *group, hal_iommu_domain_t *domain) {
    (void)group;
    (void)domain;
    return -1; // -ENOSYS
}

int hal_iommu_group_detach(hal_iommu_group_t *group) {
    (void)group;
    return -1; // -ENOSYS
}

int hal_iommu_map(hal_iommu_domain_t *domain,
                  uint64_t iova, uint64_t phys, size_t size, uint64_t prot) {
    (void)domain;
    (void)iova;
    (void)phys;
    (void)size;
    (void)prot;
    return -1; // -ENOSYS
}

int hal_iommu_unmap(hal_iommu_domain_t *domain,
                    uint64_t iova, size_t size) {
    (void)domain;
    (void)iova;
    (void)size;
    return -1; // -ENOSYS
}

int hal_iommu_block_device(bharat_device_t *dev) {
    (void)dev;
    return -1; // -ENOSYS
}

int hal_iommu_get_group(bharat_device_t *dev, hal_iommu_group_t **out_group) {
    (void)dev;
    (void)out_group;
    return -1; // -ENOSYS
}