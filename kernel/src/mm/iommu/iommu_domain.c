#include "mm/iommu.h"
#include "hal/hal_iommu.h"
#include "slab.h"

extern const hal_iommu_ops_t *hal_iommu_get_ops(void);

iommu_domain_t *iommu_domain_create(uint32_t flags) {
    if (!iommu_available()) return NULL;

    const hal_iommu_ops_t *ops = hal_iommu_get_ops();
    if (ops && ops->domain_create) {
        iommu_domain_t *dom = kmalloc(sizeof(iommu_domain_t));
        if (dom) {
            dom->flags = flags;
            dom->hal_priv = NULL;
            int ret = ops->domain_create(dom);
            if (ret == 0) return dom;
            kfree(dom);
        }
    }
    return NULL;
}

void iommu_domain_destroy(iommu_domain_t *dom) {
    if (!dom || !iommu_available()) return;

    const hal_iommu_ops_t *ops = hal_iommu_get_ops();
    if (ops && ops->domain_destroy) {
        ops->domain_destroy(dom);
    }
    kfree(dom);
}

int iommu_invalidate_domain(iommu_domain_t *dom) {
    if (!dom || !iommu_available()) return -1;

    const hal_iommu_ops_t *ops = hal_iommu_get_ops();
    if (ops && ops->invalidate_domain) {
        return ops->invalidate_domain(dom);
    }
    return -1;
}

int iommu_invalidate_range(iommu_domain_t *dom, uintptr_t iova, size_t len) {
    if (!dom || !iommu_available()) return -1;

    const hal_iommu_ops_t *ops = hal_iommu_get_ops();
    if (ops && ops->invalidate_range) {
        return ops->invalidate_range(dom, iova, len);
    }
    return -1;
}
