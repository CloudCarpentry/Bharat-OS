#include "mm/iommu.h"
#include "hal/hal_iommu.h"
#include "slab.h"

extern const hal_iommu_ops_t *hal_iommu_get_ops(void);

int iommu_map(iommu_domain_t *dom, uintptr_t iova, uint64_t pa,
              size_t len, uint64_t prot, uint64_t flags) {
    if (!dom || len == 0 || !iommu_available()) {
        return -1;
    }

    const hal_iommu_ops_t *ops = hal_iommu_get_ops();
    if (ops && ops->map) {
        return ops->map(dom, iova, pa, len, prot, flags);
    }

    return -1;
}

int iommu_unmap(iommu_domain_t *dom, uintptr_t iova, size_t len) {
    if (!dom || len == 0 || !iommu_available()) {
        return -1;
    }

    const hal_iommu_ops_t *ops = hal_iommu_get_ops();
    if (ops && ops->unmap) {
        return ops->unmap(dom, iova, len);
    }

    return -1;
}
