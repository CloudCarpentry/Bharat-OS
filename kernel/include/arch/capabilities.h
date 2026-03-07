#ifndef BHARAT_ARCH_CAPABILITIES_H
#define BHARAT_ARCH_CAPABILITIES_H

#include "bharat_config.h"

/*
 * ISA Capability Layer
 * AVX2, NEON, SVE, RVV, crypto extensions.
 * Allows generic fallback + optimized dispatch.
 */

static inline int arch_has_feature_avx2(void) {
#if defined(BHARAT_ISA_FEATURE_AVX2)
    return 1;
#else
    return 0;
#endif
}

static inline int arch_has_feature_fma(void) {
#if defined(BHARAT_ISA_FEATURE_FMA)
    return 1;
#else
    return 0;
#endif
}

static inline int arch_has_feature_aes(void) {
#if defined(BHARAT_ISA_FEATURE_AES)
    return 1;
#else
    return 0;
#endif
}

static inline int arch_has_feature_vector(void) {
#if defined(BHARAT_ISA_FEATURE_V)
    return 1;
#else
    return 0;
#endif
}

static inline int arch_has_feature_crypto(void) {
#if defined(BHARAT_ISA_FEATURE_K) || defined(BHARAT_ISA_FEATURE_ZBA) || defined(BHARAT_ISA_FEATURE_ZBB)
    return 1;
#else
    return 0;
#endif
}

#endif /* BHARAT_ARCH_CAPABILITIES_H */
