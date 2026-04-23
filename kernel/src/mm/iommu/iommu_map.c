#include "mm/iommu.h"
#include "hal/hal_iommu.h"
#include "slab.h"
#include "capability.h"
#include "sched/sched.h"

extern const hal_iommu_ops_t *hal_iommu_get_ops(void);

int iommu_map(iommu_domain_t *dom, uintptr_t iova, uint64_t pa,
              size_t len, uint64_t prot, uint64_t flags) {
    if (!dom || len == 0 || !iommu_available()) {
        return -1;
    }

    capability_table_t *ct = sched_current_cap_table();
    if (ct) {
        bool found = false;
        for (int i = 0; i < 64; i++) {
            if (ct->entries[i].in_use && ct->entries[i].type == CAP_TYPE_DMA_DOMAIN &&
                (ct->entries[i].rights & CAP_RIGHT_WRITE)) { // Need WRITE right to map
                if (ct->entries[i].object_ref == (uint64_t)dom) {
                    found = true;
                    break;
                }
            }
        }
        if (!found) return -100; // Unauthorized
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

    capability_table_t *ct = sched_current_cap_table();
    if (ct) {
        bool found = false;
        for (int i = 0; i < 64; i++) {
            if (ct->entries[i].in_use && ct->entries[i].type == CAP_TYPE_DMA_DOMAIN &&
                (ct->entries[i].rights & CAP_RIGHT_WRITE)) {
                if (ct->entries[i].object_ref == (uint64_t)dom) {
                    found = true;
                    break;
                }
            }
        }
        if (!found) return -100; // Unauthorized
    }

    const hal_iommu_ops_t *ops = hal_iommu_get_ops();
    if (ops && ops->unmap) {
        return ops->unmap(dom, iova, len);
    }

    return -1;
}
