#include "mm/iommu.h"
#include "kernel/status.h"

// Fallback implementation for when BHARAT_ENABLE_IOMMU is disabled.

bool iommu_available(void) {
    return false;
}

int iommu_map(iommu_domain_t *dom, uintptr_t iova, uint64_t pa,
              size_t len, uint64_t prot, uint64_t flags) {
    (void)dom;
    (void)iova;
    (void)pa;
    (void)len;
    (void)prot;
    (void)flags;
    return -1; // -ENOTSUP
}

int iommu_unmap(iommu_domain_t *dom, uintptr_t iova, size_t len) {
    (void)dom;
    (void)iova;
    (void)len;
    return -1; // -ENOTSUP
}
