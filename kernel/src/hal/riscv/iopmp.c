#include "hal/iommu.h"
#include <stddef.h>

// Mock IOPMP / RISC-V IOMMU operations
static int iopmp_init(void) {
    // Locate via FDT/BSP
    return 0;
}

static int iopmp_domain_create(const bharat_iommu_domain_config_t *cfg, hal_iommu_domain_t **out) {
    (void)cfg;
    if (out) *out = (hal_iommu_domain_t*)0x3456;
    return 0;
}

static int iopmp_domain_destroy(hal_iommu_domain_t *domain) {
    (void)domain;
    return 0;
}

static int iopmp_group_attach(hal_iommu_group_t *group, hal_iommu_domain_t *domain) {
    (void)group;
    (void)domain;
    return 0;
}

static int iopmp_group_detach(hal_iommu_group_t *group) {
    (void)group;
    return 0;
}

static int iopmp_map(hal_iommu_domain_t *domain, uint64_t iova, uint64_t phys, size_t size, uint64_t prot) {
    (void)domain;
    (void)iova;
    (void)phys;
    (void)size;
    (void)prot;
    return 0;
}

static int iopmp_unmap(hal_iommu_domain_t *domain, uint64_t iova, size_t size) {
    (void)domain;
    (void)iova;
    (void)size;
    return 0;
}

static int iopmp_block_device(bharat_device_t *dev) {
    (void)dev;
    return 0;
}

static int iopmp_get_group(bharat_device_t *dev, hal_iommu_group_t **out_group) {
    (void)dev;
    if (out_group) *out_group = (hal_iommu_group_t*)0x789A;
    return 0;
}

static hal_iommu_ops_t iopmp_ops = {
    .init = iopmp_init,
    .domain_create = iopmp_domain_create,
    .domain_destroy = iopmp_domain_destroy,
    .group_attach = iopmp_group_attach,
    .group_detach = iopmp_group_detach,
    .map = iopmp_map,
    .unmap = iopmp_unmap,
    .block_device = iopmp_block_device,
    .get_group = iopmp_get_group
};

void riscv_iommu_detect(void) {
    // Real implementation checks FDT for iopmp.
    hal_iommu_register_ops(&iopmp_ops);
}
