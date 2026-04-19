#include "hal/hal_iommu.h"
#include "console/console_core.h"
#include "kernel/status.h"

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

int iommu_map(iommu_domain_t *dom, uintptr_t iova, uint64_t pa,
              size_t len, uint64_t prot, uint64_t flags) {
    return K_ERR_UNSUPPORTED;
}

int iommu_unmap(iommu_domain_t *dom, uintptr_t iova, size_t len) {
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
