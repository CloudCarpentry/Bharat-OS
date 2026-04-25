#ifndef BHARAT_ARCH_COMMON_ACCEL_CAPS_PUBLISH_H
#define BHARAT_ARCH_COMMON_ACCEL_CAPS_PUBLISH_H

#include "arch/arch_cpu_caps.h"
#include "hal/hal_discovery.h"

void arch_accel_caps_publish_target(accel_discovery_t *accel,
                                    const arch_cpu_caps_record_t *caps_all,
                                    const arch_cpu_caps_record_t *caps_any);

#endif // BHARAT_ARCH_COMMON_ACCEL_CAPS_PUBLISH_H
