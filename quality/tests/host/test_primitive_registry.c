#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "kernel/primitive.h"
#include "hal/hal_hw_caps.h"

// Mock HAL implementation for host test
static hal_hw_caps_t g_test_caps;
const hal_hw_caps_t *hal_get_internal_hw_caps(void) {
    return &g_test_caps;
}

void test_primitive_registry(void) {
    printf("Testing primitive registry...\n");

    hal_hw_caps_t caps = {0};
    caps.has_mmu = true;
    caps.has_iommu = true;
    caps.has_high_res_timer = true;

    // Initialize registry
    kstatus_t status = bh_kernel_primitive_registry_init(&caps);
    assert(status == K_OK);

    // Test availability
    assert(bh_kernel_primitive_available(BH_PRIMITIVE_MEMORY));
    assert(bh_kernel_primitive_available(BH_PRIMITIVE_DMA));
    assert(bh_kernel_primitive_available(BH_PRIMITIVE_TIMER));
    assert(!bh_kernel_primitive_available(BH_PRIMITIVE_ACCEL));

    // Test support levels
    assert(bh_kernel_primitive_get_support_level(BH_PRIMITIVE_MEMORY) == BH_PRIMITIVE_HARDWARE_ENFORCED);
    assert(bh_kernel_primitive_get_support_level(BH_PRIMITIVE_DMA) == BH_PRIMITIVE_HARDWARE_ENFORCED);
    assert(bh_kernel_primitive_get_support_level(BH_PRIMITIVE_TIMER) == BH_PRIMITIVE_HARDWARE_ASSISTED);
    assert(bh_kernel_primitive_get_support_level(BH_PRIMITIVE_SCHED) == BH_PRIMITIVE_SOFTWARE_FALLBACK);

    printf("Primitive registry tests passed!\n");
}

int main() {
    test_primitive_registry();
    return 0;
}
