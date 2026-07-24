#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "hal/hal_capabilities.h"
#include "kernel/status.h"

// Forward declaration of the validation helper (normally in hal_unsupported.c)
extern bool hal_validate_arch_capabilities(const hal_arch_capabilities_t *caps,
                                          const hal_arch_capabilities_t *required);

void test_host_capabilities(void) {
    const hal_arch_capabilities_t *caps = hal_get_arch_capabilities();

    printf("Testing Architecture: %s\n", caps->arch_name);
    assert(caps->arch_bits == 32 || caps->arch_bits == 64);
    assert(caps->support_level <= BH_ARCH_SUPPORT_PRODUCTION_SUPPORTED);

    if (caps->support_level >= BH_ARCH_SUPPORT_RUNTIME_SUPPORTED) {
        assert(caps->has_irq_controller == true);
        assert(caps->has_monotonic_timer == true);
    }
}

void test_capability_validation(void) {
    hal_arch_capabilities_t caps = {
        .support_level = BH_ARCH_SUPPORT_RUNTIME_SUPPORTED,
        .memory_model = BH_MEMORY_MODEL_MMU_FULL,
        .has_smp = true
    };

    hal_arch_capabilities_t req_ok = {
        .support_level = BH_ARCH_SUPPORT_BOOT_SUPPORTED,
        .memory_model = BH_MEMORY_MODEL_MMU_LITE,
        .has_smp = true
    };

    hal_arch_capabilities_t req_fail_tier = {
        .support_level = BH_ARCH_SUPPORT_PRODUCTION_SUPPORTED
    };

    hal_arch_capabilities_t req_fail_feature = {
        .has_iommu = true
    };

    assert(hal_validate_arch_capabilities(&caps, &req_ok) == true);
    assert(hal_validate_arch_capabilities(&caps, &req_fail_tier) == false);
    assert(hal_validate_arch_capabilities(&caps, &req_fail_feature) == false);

    printf("Capability validation logic: PASSED\n");
}

int main(void) {
    printf("Starting HAL Capability Tests...\n");

    test_host_capabilities();
    test_capability_validation();

    printf("All HAL Capability Tests: PASSED\n");
    return 0;
}
