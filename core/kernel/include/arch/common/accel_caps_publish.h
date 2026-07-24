#ifndef BHARAT_ARCH_COMMON_ACCEL_CAPS_PUBLISH_H
#define BHARAT_ARCH_COMMON_ACCEL_CAPS_PUBLISH_H

#include "arch/arch_cpu_caps.h"
#include "hal/hal_discovery.h"

void arch_accel_caps_publish_target(accel_discovery_t *accel,
                                    const arch_cpu_caps_record_t *caps_all,
                                    const arch_cpu_caps_record_t *caps_any);

static inline void accel_set_mask(uint64_t *mask, hal_accel_feature_t feat,
                                  const arch_cpu_caps_t *caps, int arch_feat) {
    if (feat >= HAL_ACCEL_FEAT__COUNT) {
        return;
    }
    if (arch_cpu_caps_test(caps, arch_feat)) {
        *mask |= (1ULL << (uint32_t)feat);
    }
}

#endif // BHARAT_ARCH_COMMON_ACCEL_CAPS_PUBLISH_H
