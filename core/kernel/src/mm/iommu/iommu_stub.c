#include "mm/iommu.h"
#include "hal/hal_iommu.h"
#include "console/console_core.h"
#include "kernel/status.h"

static const hal_iommu_ops_t *g_stub_iommu_ops = NULL;

void hal_iommu_set_ops(const hal_iommu_ops_t *ops) {
    g_stub_iommu_ops = ops;
}

const hal_iommu_ops_t *hal_iommu_get_ops(void) {
    return g_stub_iommu_ops;
}

int iommu_init(void) {
    console_write_raw("IOMMU: Subsystem disabled/stubbed via build configuration\n", 58);
    return K_OK; // Success, just no-op
}

bool iommu_available(void) {
    return false;
}

bool iommu_enabled(void) {
    return false;
}

iommu_domain_t *iommu_domain_create(uint32_t flags) {
    return NULL;
}

void iommu_domain_destroy(iommu_domain_t *dom) {
    // No-op
}

int iommu_attach_device(iommu_domain_t *dom, iommu_device_t *dev) {
    return K_ERR_UNSUPPORTED;
}

int iommu_detach_device(iommu_device_t *dev) {
    return K_ERR_UNSUPPORTED;
}

kstatus_t iommu_map(
    bh_iommu_domain_t *domain,
    uintptr_t iova,
    uintptr_t paddr,
    size_t len,
    uint32_t flags
) {
    (void)domain; (void)iova; (void)paddr; (void)len; (void)flags;
    return K_ERR_UNSUPPORTED;
}

kstatus_t iommu_unmap(
    bh_iommu_domain_t *domain,
    uintptr_t iova,
    size_t len
) {
    (void)domain; (void)iova; (void)len;
    return K_ERR_UNSUPPORTED;
}

int iommu_invalidate_domain(iommu_domain_t *dom) {
    return K_ERR_UNSUPPORTED;
}

int iommu_invalidate_range(iommu_domain_t *dom, uintptr_t iova, size_t len) {
    return K_ERR_UNSUPPORTED;
}

int iommu_get_dma_mode(iommu_device_t *dev, dma_translation_mode_t *mode_out) {
    if (!mode_out) return K_ERR_INVALID_ARG;

    // IOMMU is absent, so fallback to DIRECT mode.
    *mode_out = DMA_TRANSLATION_DIRECT;
    return K_OK;
}
