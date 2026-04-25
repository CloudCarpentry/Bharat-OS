#ifndef BHARAT_ARCH_CAPABILITIES_H
#define BHARAT_ARCH_CAPABILITIES_H

#include "arch/arch_cpu_caps.h"

static inline void arch_capabilities_init(void) {
    arch_cpu_caps_init();
}

static inline int arch_has_feature_avx2(void) {
#if defined(__x86_64__)
    return arch_cpu_has_system_all(ARCH_CPU_FEAT_X86_AVX2);
#else
    return 0;
#endif
}

static inline int arch_has_feature_fma(void) {
#if defined(__x86_64__)
    return arch_cpu_has_system_all(ARCH_CPU_FEAT_X86_FMA);
#else
    return 0;
#endif
}

static inline int arch_has_feature_aes(void) {
    return arch_cpu_has_system_all(ARCH_CPU_FEAT_COMMON_AES);
}

static inline int arch_has_feature_vector(void) {
    return arch_cpu_has_system_all(ARCH_CPU_FEAT_COMMON_VECTOR);
}

static inline int arch_has_feature_crypto(void) {
    return arch_cpu_has_system_all(ARCH_CPU_FEAT_COMMON_AES) ||
           arch_cpu_has_system_all(ARCH_CPU_FEAT_COMMON_SHA) ||
           arch_cpu_has_system_all(ARCH_CPU_FEAT_COMMON_PMULL);
}

#endif /* BHARAT_ARCH_CAPABILITIES_H */
