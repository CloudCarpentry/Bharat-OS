#ifndef BHARAT_UAPI_CAPABILITY_FEATURE_CLASS_H
#define BHARAT_UAPI_CAPABILITY_FEATURE_CLASS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * High-level abstract feature classes derived from underlying hardware capabilities.
 * These classes define expected behavior and acceleration domains, shielding the OS
 * from vendor-specific or ISA-specific implementations.
 */
typedef enum {
    CLASS_NONE              = 0,
    CLASS_COMPUTE_VECTOR    = (1 << 0),  /**< SIMD, matrix multiplication, vector processing */
    CLASS_CRYPTO            = (1 << 1),  /**< Hardware hashing, symmetric/asymmetric crypto */
    CLASS_TENSOR_ML         = (1 << 2),  /**< Neural processing, discrete NPUs */
    CLASS_PACKET_OFFLOAD    = (1 << 3),  /**< Network checksums, segmentation, IPsec offload */
    CLASS_MEDIA_DISPLAY     = (1 << 4),  /**< GPUs, video codecs, display controllers */
    CLASS_SENSOR_ACTUATOR   = (1 << 5),  /**< Real-time sensor hubs, motor control (PWM) */
    CLASS_STORAGE_ACCEL     = (1 << 6),  /**< NVMe acceleration, DMA storage offload */
    CLASS_TRUST_SECURITY    = (1 << 7),  /**< Secure enclaves, root of trust, secure keys */
    CLASS_TIMING_RT         = (1 << 8),  /**< Precision timing, TSN, deterministic limits */
    CLASS_POWER_THERMAL     = (1 << 9)   /**< DVFS, sleep states, thermal management */
} bharat_feature_class_t;

#ifdef __cplusplus
}
#endif

#endif /* BHARAT_UAPI_CAPABILITY_FEATURE_CLASS_H */
