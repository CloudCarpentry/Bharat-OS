#ifndef BHARAT_ACCELERATORS_SUBSYSTEM_H
#define BHARAT_ACCELERATORS_SUBSYSTEM_H

#include "bharat_config.h"

/*
 * Accelerator Subsystem Layer
 * Manages GPU, NPU, DSP, Crypto engines, VPU.
 * Provides interfaces for DMA, memory sharing, synchronization.
 */

struct accelerator_info {
    const char *name;
    int is_enabled;
};

static inline struct accelerator_info get_gpu_info(void) {
    struct accelerator_info info = { "GPU", 0 };
#if defined(BHARAT_ACCEL_GPU)
    info.is_enabled = 1;
#endif
    return info;
}

static inline struct accelerator_info get_npu_info(void) {
    struct accelerator_info info = { "NPU", 0 };
#if defined(BHARAT_ACCEL_NPU)
    info.is_enabled = 1;
#endif
    return info;
}

static inline struct accelerator_info get_dsp_info(void) {
    struct accelerator_info info = { "DSP", 0 };
#if defined(BHARAT_ACCEL_DSP)
    info.is_enabled = 1;
#endif
    return info;
}

static inline struct accelerator_info get_crypto_info(void) {
    struct accelerator_info info = { "Crypto Engine", 0 };
#if defined(BHARAT_ACCEL_CRYPTO)
    info.is_enabled = 1;
#endif
    return info;
}

static inline struct accelerator_info get_vpu_info(void) {
    struct accelerator_info info = { "VPU", 0 };
#if defined(BHARAT_ACCEL_VPU)
    info.is_enabled = 1;
#endif
    return info;
}

#endif /* BHARAT_ACCELERATORS_SUBSYSTEM_H */
