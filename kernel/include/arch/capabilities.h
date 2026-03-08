#ifndef BHARAT_ARCH_CAPABILITIES_H
#define BHARAT_ARCH_CAPABILITIES_H

#include <stdint.h>
#include "bharat_config.h"

/*
 * ISA Capability Layer
 * AVX2, NEON, SVE, RVV, crypto extensions.
 * Allows generic fallback + optimized dispatch via runtime capability descriptor.
 */

typedef struct {
    uint32_t has_avx2   : 1;
    uint32_t has_fma    : 1;
    uint32_t has_aes    : 1;
    uint32_t has_vector : 1;
    uint32_t has_crypto : 1;
} arch_capabilities_t;

extern arch_capabilities_t g_arch_caps;

// Call early in boot to detect features via CPUID or dtb/misa
void arch_capabilities_init(void);

static inline int arch_has_feature_avx2(void) {
    return g_arch_caps.has_avx2;
}

static inline int arch_has_feature_fma(void) {
    return g_arch_caps.has_fma;
}

static inline int arch_has_feature_aes(void) {
    return g_arch_caps.has_aes;
}

static inline int arch_has_feature_vector(void) {
    return g_arch_caps.has_vector;
}

static inline int arch_has_feature_crypto(void) {
    return g_arch_caps.has_crypto;
}

#endif /* BHARAT_ARCH_CAPABILITIES_H */
