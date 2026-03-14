#include "hal/iommu.h"
#include <stddef.h>

static hal_iommu_ops_t *g_iommu_ops = NULL;

void hal_iommu_register_ops(hal_iommu_ops_t *ops) {
    g_iommu_ops = ops;
}

int hal_iommu_init(void) {
    if (g_iommu_ops && g_iommu_ops->init) {
        return g_iommu_ops->init();
    }
    return -1; // -ENOSYS
}

int hal_iommu_domain_create(const bharat_iommu_domain_config_t *cfg,
                            hal_iommu_domain_t **out) {
    if (g_iommu_ops && g_iommu_ops->domain_create) {
        return g_iommu_ops->domain_create(cfg, out);
    }
    return -1; // -ENOSYS
}

int hal_iommu_domain_destroy(hal_iommu_domain_t *domain) {
    if (g_iommu_ops && g_iommu_ops->domain_destroy) {
        return g_iommu_ops->domain_destroy(domain);
    }
    return -1; // -ENOSYS
}

int hal_iommu_group_attach(hal_iommu_group_t *group, hal_iommu_domain_t *domain) {
    if (g_iommu_ops && g_iommu_ops->group_attach) {
        return g_iommu_ops->group_attach(group, domain);
    }
    return -1; // -ENOSYS
}

int hal_iommu_group_detach(hal_iommu_group_t *group) {
    if (g_iommu_ops && g_iommu_ops->group_detach) {
        return g_iommu_ops->group_detach(group);
    }
    return -1; // -ENOSYS
}

int hal_iommu_map(hal_iommu_domain_t *domain,
                  uint64_t iova, uint64_t phys, size_t size, uint64_t prot) {
    if (g_iommu_ops && g_iommu_ops->map) {
        return g_iommu_ops->map(domain, iova, phys, size, prot);
    }
    return -1; // -ENOSYS
}

int hal_iommu_unmap(hal_iommu_domain_t *domain,
                    uint64_t iova, size_t size) {
    if (g_iommu_ops && g_iommu_ops->unmap) {
        return g_iommu_ops->unmap(domain, iova, size);
    }
    return -1; // -ENOSYS
}

int hal_iommu_block_device(bharat_device_t *dev) {
    if (g_iommu_ops && g_iommu_ops->block_device) {
        return g_iommu_ops->block_device(dev);
    }
    return -1; // -ENOSYS
}

int hal_iommu_get_group(bharat_device_t *dev, hal_iommu_group_t **out_group) {
    if (g_iommu_ops && g_iommu_ops->get_group) {
        return g_iommu_ops->get_group(dev, out_group);
    }
    return -1; // -ENOSYS
}