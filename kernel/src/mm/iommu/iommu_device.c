#include "mm/iommu.h"
#include "hal/hal_iommu.h"
#include "slab.h"

extern const hal_iommu_ops_t *hal_iommu_get_ops(void);

int iommu_attach_device(iommu_domain_t *dom, iommu_device_t *dev) {
    if (!dom || !dev) return -1;

    // If not managed, simply return unsupported explicitly as it bypasses the active domain
    if (dev->mode != IOMMU_DEV_MANAGED) {
        return -1;
    }

    if (!iommu_available()) return -1;

    const hal_iommu_ops_t *ops = hal_iommu_get_ops();
    if (ops && ops->attach_device) {
        return ops->attach_device(dom, dev);
    }
    return -1;
}

int iommu_detach_device(iommu_device_t *dev) {
    if (!dev) return -1;
    if (dev->mode != IOMMU_DEV_MANAGED) return -1;
    if (!iommu_available()) return -1;

    const hal_iommu_ops_t *ops = hal_iommu_get_ops();
    if (ops && ops->detach_device) {
        return ops->detach_device(dev);
    }
    return -1;
}
