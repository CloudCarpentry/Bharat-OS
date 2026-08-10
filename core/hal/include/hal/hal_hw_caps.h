#ifndef BHARAT_HAL_HW_CAPS_H
#define BHARAT_HAL_HW_CAPS_H

#include <stdint.h>
#include <stdbool.h>
#include "kernel/status.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Canonical Internal Hardware Capability Descriptor.
 * Used by the kernel and HAL to make primitive enablement decisions.
 */
typedef struct hal_hw_caps {
    bool has_mmu;
    bool has_mpu;
    bool has_iommu;
    bool has_dma_coherent;
    bool has_cache_maintenance;
    bool has_local_apic_or_gic_or_plic;
    bool has_high_res_timer;
    bool has_atomic_64;
    bool has_vector;
    bool has_crypto_accel;
    bool has_accel_device;
    uint32_t max_cpus;
    uint32_t page_granule;
    uint32_t cache_line_size;
} hal_hw_caps_t;

typedef enum {
    HAL_CAPS_UNINITIALIZED = 0,
    HAL_CAPS_RAW_DISCOVERED,
    HAL_CAPS_CPU_FINALIZED,
    HAL_CAPS_FROZEN
} hal_caps_state_t;

/**
 * Get the internal hardware capabilities.
 *
 * @return A pointer to the global hardware capabilities structure.
 */
const hal_hw_caps_t *hal_get_internal_hw_caps(void);

hal_caps_state_t hal_hw_caps_state(void);
bool hal_hw_caps_is_frozen(void);
kstatus_t hal_hw_caps_publish_raw(const hal_hw_caps_t *caps);
kstatus_t hal_hw_caps_publish_cpu(void);
kstatus_t hal_hw_caps_finalize(void);

#ifdef __cplusplus
}
#endif

#endif /* BHARAT_HAL_HW_CAPS_H */
