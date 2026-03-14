#include "hal/iommu.h"
#include <stddef.h>

// Mock SMMU v2/v3 operations
static int smmu_init(void) {
    // Locate via FDT
    return 0;
}

static int smmu_domain_create(const bharat_iommu_domain_config_t *cfg, hal_iommu_domain_t **out) {
    (void)cfg;
    if (out) *out = (hal_iommu_domain_t*)0x2345;
    return 0;
}

static int smmu_domain_destroy(hal_iommu_domain_t *domain) {
    (void)domain;
    return 0;
}

static int smmu_group_attach(hal_iommu_group_t *group, hal_iommu_domain_t *domain) {
    (void)group;
    (void)domain;
    return 0;
}

static int smmu_group_detach(hal_iommu_group_t *group) {
    (void)group;
    return 0;
}

static int smmu_map(hal_iommu_domain_t *domain, uint64_t iova, uint64_t phys, size_t size, uint64_t prot) {
    (void)domain;
    (void)iova;
    (void)phys;
    (void)size;
    (void)prot;
    return 0;
}

static int smmu_unmap(hal_iommu_domain_t *domain, uint64_t iova, size_t size) {
    (void)domain;
    (void)iova;
    (void)size;
    return 0;
}

static int smmu_block_device(bharat_device_t *dev) {
    (void)dev;
    return 0;
}

static int smmu_get_group(bharat_device_t *dev, hal_iommu_group_t **out_group) {
    (void)dev;
    if (out_group) *out_group = (hal_iommu_group_t*)0x6789;
    return 0;
}

static hal_iommu_ops_t smmu_ops = {
    .init = smmu_init,
    .domain_create = smmu_domain_create,
    .domain_destroy = smmu_domain_destroy,
    .group_attach = smmu_group_attach,
    .group_detach = smmu_group_detach,
    .map = smmu_map,
    .unmap = smmu_unmap,
    .block_device = smmu_block_device,
    .get_group = smmu_get_group
};

void arm64_iommu_detect(void) {
    // Real implementation checks FDT for smmu.
    hal_iommu_register_ops(&smmu_ops);
}
