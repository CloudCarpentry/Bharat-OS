#include "mm/iommu.h"
#include "hal/hal_iommu.h"
#include "slab.h"
#include "capability.h"
#include "sched/sched.h"

extern const hal_iommu_ops_t *hal_iommu_get_ops(void);

int iommu_attach_device(iommu_domain_t *dom, iommu_device_t *dev) {
    if (!dom || !dev) return -1;

    // Capability check: Current process must have BIND/ADMIN right on the DMA domain
    capability_table_t *ct = sched_current_cap_table();
    if (ct) {
        capability_entry_t entry;
        // Search for a capability that gives access to the domain
        // In a real system, the dom would be referenced by a handle.
        // For now, we simulate a check if any CAP_TYPE_DMA_DOMAIN exists with BIND right.
        // A better way would be to pass the handle to this function.
        bool found = false;
        for (int i = 0; i < 64; i++) {
            if (ct->entries[i].in_use && ct->entries[i].type == CAP_TYPE_DMA_DOMAIN &&
                (ct->entries[i].rights & CAP_RIGHT_BIND)) {
                if (ct->entries[i].object_ref == (uint64_t)dom) {
                    found = true;
                    break;
                }
            }
        }
        if (!found) return -100; // Unauthorized
    }

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
