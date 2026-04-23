#ifndef BHARAT_UAPI_CAPABILITY_HW_CAPS_H
#define BHARAT_UAPI_CAPABILITY_HW_CAPS_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Capability states describing the availability and health of a feature.
 */
typedef enum {
    HW_CAP_STATE_ABSENT = 0,
    HW_CAP_STATE_PRESENT,
    HW_CAP_STATE_OPTIONAL,
    HW_CAP_STATE_REQUIRED,
    HW_CAP_STATE_DEGRADED
} hw_cap_state_t;

/**
 * CPU Feature Capabilities
 */
typedef struct {
    hw_cap_state_t atomics;
    hw_cap_state_t vector_simd;
    hw_cap_state_t crypto;
    hw_cap_state_t virtualization;
    hw_cap_state_t precision_timer;
    hw_cap_state_t cache_coherency;
} hw_caps_cpu_t;

/**
 * Memory and Protection Capabilities
 */
typedef struct {
    hw_cap_state_t mpu;
    hw_cap_state_t mmu_lite;
    hw_cap_state_t mmu;
    hw_cap_state_t iommu;
    hw_cap_state_t dma_coherency;
} hw_caps_memory_t;

/**
 * SoC and Peripheral Capabilities
 */
typedef struct {
    hw_cap_state_t dma;
    hw_cap_state_t watchdog;
    hw_cap_state_t gpu;
    hw_cap_state_t npu;
    hw_cap_state_t isp;
    hw_cap_state_t video_codec;
    hw_cap_state_t tsn_can;
    hw_cap_state_t radio;
    hw_cap_state_t secure_enclave;
} hw_caps_soc_t;

/**
 * Canonical Hardware Capability Record.
 * One normalized struct populated during boot discovery.
 */
typedef struct {
    hw_caps_cpu_t    cpu;
    hw_caps_memory_t memory;
    hw_caps_soc_t    soc;
} bharat_hw_caps_t;

#ifdef __cplusplus
}
#endif

#endif /* BHARAT_UAPI_CAPABILITY_HW_CAPS_H */
