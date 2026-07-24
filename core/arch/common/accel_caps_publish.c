#include "arch/common/accel_caps_publish.h"

#include <stdint.h>

// Weak stub for arch-specific publish logic
void __attribute__((weak)) arch_accel_caps_publish_target_impl(accel_discovery_t *accel,
                                    const arch_cpu_caps_record_t *caps_all,
                                    const arch_cpu_caps_record_t *caps_any) {
    (void)accel; (void)caps_all; (void)caps_any;
}

void arch_accel_caps_publish_target(accel_discovery_t *accel,
                                    const arch_cpu_caps_record_t *caps_all,
                                    const arch_cpu_caps_record_t *caps_any) {
    if (!accel || !caps_all || !caps_any) {
        return;
    }

    arch_accel_caps_publish_target_impl(accel, caps_all, caps_any);
}
