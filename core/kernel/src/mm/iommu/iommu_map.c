#include "mm/iommu.h"
#include "hal/hal_iommu.h"
#include "slab.h"
#include "capability.h"
#include "sched/sched.h"

extern const hal_iommu_ops_t *hal_iommu_get_ops(void);

kstatus_t iommu_map(bh_iommu_domain_t *dom, uintptr_t iova, uintptr_t paddr,
              size_t len, uint32_t flags) {
    (void)flags;
    if (!dom || len == 0 || !iommu_available()) {
        return K_ERR_INVALID_ARG;
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
        if (!found) return K_ERR_DENIED; // Unauthorized
    }

    const hal_iommu_ops_t *ops = hal_iommu_get_ops();
    if (ops && ops->map) {
        // Map flags to prot/flags for old HAL API if needed,
        // but here we keep it simple for the migration.
        return ops->map(dom, iova, paddr, len, 0, flags);
    }

    return K_ERR_UNSUPPORTED;
}

kstatus_t iommu_unmap(bh_iommu_domain_t *dom, uintptr_t iova, size_t len) {
    if (!dom || len == 0 || !iommu_available()) {
        return K_ERR_INVALID_ARG;
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
        if (!found) return K_ERR_DENIED; // Unauthorized
    }

    const hal_iommu_ops_t *ops = hal_iommu_get_ops();
    if (ops && ops->unmap) {
        return ops->unmap(dom, iova, len);
    }

    return K_ERR_UNSUPPORTED;
}
