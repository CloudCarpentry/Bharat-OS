#ifndef BHARAT_MM_IOMMU_H
#define BHARAT_MM_IOMMU_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "kernel/status.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    IOMMU_DEV_UNMANAGED = 0,
    IOMMU_DEV_MANAGED,
    IOMMU_DEV_BYPASS,
    IOMMU_DEV_IDENTITY,
    IOMMU_DEV_BLOCKED
} iommu_device_mode_t;

typedef enum {
    DMA_TRANSLATION_NONE = 0,
    DMA_TRANSLATION_DIRECT,
    DMA_TRANSLATION_IDENTITY,
    DMA_TRANSLATION_IOMMU,
    DMA_TRANSLATION_BOUNCE,
    DMA_TRANSLATION_BLOCKED
} dma_translation_mode_t;

// Opaque generic domain and device representations
typedef struct iommu_domain {
    uint32_t flags;
    void *hal_priv; // Backend-specific context
} iommu_domain_t;

typedef iommu_domain_t bh_iommu_domain_t;

typedef struct iommu_device {
    uint64_t bdf; // Bus/Device/Function or similar routing ID
    iommu_device_mode_t mode;
    void *hal_priv; // Backend-specific context
} iommu_device_t;

typedef struct iommu_fault_info {
    iommu_device_t *dev;
    uintptr_t iova;
    uint64_t access;
    uint64_t reason;
    bool recoverable;
} iommu_fault_info_t;

int iommu_init(void);
int iommu_fault_init(uint32_t irq_vector);

bool iommu_available(void);
bool iommu_enabled(void);

iommu_domain_t *iommu_domain_create(uint32_t flags);
void iommu_domain_destroy(iommu_domain_t *dom);

int iommu_attach_device(iommu_domain_t *dom, iommu_device_t *dev);
int iommu_detach_device(iommu_device_t *dev);

kstatus_t iommu_map(
    bh_iommu_domain_t *domain,
    uintptr_t iova,
    uintptr_t paddr,
    size_t len,
    uint32_t flags
);

kstatus_t iommu_unmap(
    bh_iommu_domain_t *domain,
    uintptr_t iova,
    size_t len
);

int iommu_invalidate_domain(iommu_domain_t *dom);
int iommu_invalidate_range(iommu_domain_t *dom, uintptr_t iova, size_t len);

int iommu_get_dma_mode(iommu_device_t *dev, dma_translation_mode_t *mode_out);

// Backend registration for hal implementations
// TODO: Needs refactor: #include directive placed mid-file for dependency/order compatibility.
#include "hal/hal_iommu.h"

#ifdef __cplusplus
}
#endif

#endif // BHARAT_MM_IOMMU_H
