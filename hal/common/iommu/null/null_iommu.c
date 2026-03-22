#include "hal/hal_iommu.h"
#include "console/console_core.h"

#define ENOTSUP 95

static int null_iommu_init(void) {
    return 0; // Success, just no capability
}

static iommu_state_t null_iommu_query_state(void) {
    return IOMMU_STATE_NONE;
}

static int null_iommu_domain_create(iommu_domain_t *dom) {
    (void)dom;
    return -ENOTSUP;
}

static void null_iommu_domain_destroy(iommu_domain_t *dom) {
    (void)dom;
}

static int null_iommu_attach_device(iommu_domain_t *dom, iommu_device_t *dev) {
    (void)dom;
    (void)dev;
    return -ENOTSUP;
}

static int null_iommu_detach_device(iommu_device_t *dev) {
    (void)dev;
    return -ENOTSUP;
}

static int null_iommu_map(iommu_domain_t *dom, uintptr_t iova, uint64_t pa,
                          size_t len, uint64_t prot, uint64_t flags) {
    (void)dom;
    (void)iova;
    (void)pa;
    (void)len;
    (void)prot;
    (void)flags;
    return -ENOTSUP;
}

static int null_iommu_unmap(iommu_domain_t *dom, uintptr_t iova, size_t len) {
    (void)dom;
    (void)iova;
    (void)len;
    return -ENOTSUP;
}

static int null_iommu_invalidate_domain(iommu_domain_t *dom) {
    (void)dom;
    return -ENOTSUP;
}

static int null_iommu_invalidate_range(iommu_domain_t *dom, uintptr_t iova, size_t len) {
    (void)dom;
    (void)iova;
    (void)len;
    return -ENOTSUP;
}

static int null_iommu_query_device_caps(iommu_device_t *dev, iommu_device_caps_t *caps) {
    (void)dev;
    if (caps) {
        caps->can_attach = false;
        caps->supports_translation = false;
        caps->supports_bypass = true;
        caps->coherent = false;
        caps->needs_identity_map = false;
    }
    return 0;
}

const hal_iommu_ops_t null_iommu_ops = {
    .init              = null_iommu_init,
    .query_state       = null_iommu_query_state,
    .domain_create     = null_iommu_domain_create,
    .domain_destroy    = null_iommu_domain_destroy,
    .attach_device     = null_iommu_attach_device,
    .detach_device     = null_iommu_detach_device,
    .map               = null_iommu_map,
    .unmap             = null_iommu_unmap,
    .invalidate_domain = null_iommu_invalidate_domain,
    .invalidate_range  = null_iommu_invalidate_range,
    .query_device_caps = null_iommu_query_device_caps,
};
