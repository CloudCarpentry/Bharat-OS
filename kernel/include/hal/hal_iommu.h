#ifndef BHARAT_HAL_HAL_IOMMU_H
#define BHARAT_HAL_HAL_IOMMU_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Forward declaration of generic types
struct iommu_domain;
typedef struct iommu_domain iommu_domain_t;

struct iommu_device;
typedef struct iommu_device iommu_device_t;

typedef enum {
    IOMMU_STATE_NONE = 0,        // no IOMMU hardware or no backend
    IOMMU_STATE_PRESENT,         // hardware detected
    IOMMU_STATE_ENABLED,         // enabled and usable
    IOMMU_STATE_BYPASS,          // present but running in bypass/pass-through mode
    IOMMU_STATE_DEGRADED         // partial support / backend incomplete
} iommu_state_t;

typedef struct iommu_device_caps {
    bool can_attach;
    bool supports_translation;
    bool supports_bypass;
    bool coherent;
    bool needs_identity_map;
} iommu_device_caps_t;

typedef struct hal_iommu_ops {
    int  (*init)(void);
    iommu_state_t (*query_state)(void);

    int  (*domain_create)(iommu_domain_t *dom);
    void (*domain_destroy)(iommu_domain_t *dom);

    int  (*attach_device)(iommu_domain_t *dom, iommu_device_t *dev);
    int  (*detach_device)(iommu_device_t *dev);

    int  (*map)(iommu_domain_t *dom, uintptr_t iova, uint64_t pa,
                size_t len, uint64_t prot, uint64_t flags);
    int  (*unmap)(iommu_domain_t *dom, uintptr_t iova, size_t len);

    int  (*invalidate_domain)(iommu_domain_t *dom);
    int  (*invalidate_range)(iommu_domain_t *dom, uintptr_t iova, size_t len);

    int  (*query_device_caps)(iommu_device_t *dev, iommu_device_caps_t *caps);
} hal_iommu_ops_t;

// Set the active IOMMU backend ops during probe
void hal_iommu_set_ops(const hal_iommu_ops_t *ops);
const hal_iommu_ops_t *hal_iommu_get_ops(void);

// Include the old file for backward compat.
// TODO: Needs refactor: #include directive placed mid-file for dependency/order compatibility.
#include "hal/iommu.h"

#ifdef __cplusplus
}
#endif

#endif // BHARAT_HAL_HAL_IOMMU_H
