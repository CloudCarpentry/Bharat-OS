#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "kernel/primitive.h"
#include "kernel/primitive_caps.h"
#include "hal/hal_hw_caps.h"
#include "hal/hal_cpu_features.h"
#include "hal/hal_tlb.h"

// Stubs for HAL
static hal_hw_caps_t mock_hw_caps = {0};
static hal_tlb_caps_t mock_tlb_caps = {0};
static hal_cpu_feature_set_t mock_cpu_caps_all = {0};
static hal_cpu_feature_set_t mock_cpu_caps_any = {0};

const hal_tlb_caps_t *hal_tlb_caps(void) {
    return &mock_tlb_caps;
}

bool hal_cpu_feature_set_system(hal_cpu_feature_scope_t scope, hal_cpu_feature_set_t *out) {
    if (scope == HAL_CPU_FEATURE_SCOPE_ALL) {
        *out = mock_cpu_caps_all;
    } else {
        *out = mock_cpu_caps_any;
    }
    return true;
}

bool hal_cpu_has_feature_current(hal_cpu_feature_t feature) {
    return false; // Not used in this test
}

void test_initialization(void) {
    printf("Testing initialization...\n");

    // Test that uninitialized registry returns false/unsupported
    assert(bh_kprim_has(BH_KPRIM_CAP_ATOMIC_64) == false);
    assert(bh_kprim_get_support_level(BH_KPRIM_CAP_ATOMIC_64) == BH_PRIMITIVE_UNSUPPORTED);

    // Mock values
    mock_hw_caps.has_atomic_64 = true;
    mock_tlb_caps.supports_page_flush = true;

    // Init
    kstatus_t status = bh_kernel_primitive_registry_init(&mock_hw_caps);
    assert(status == K_OK);

    // Test double init fails
    hal_hw_caps_t empty_caps = {0};
    status = bh_kernel_primitive_registry_init(&empty_caps);
    assert(status == K_ERR_BAD_STATE);
}

void test_queries(void) {
    printf("Testing queries...\n");

    // We expect ATOMIC_64 and TLB_PAGE_INVALIDATE to be true based on the mock
    assert(bh_kprim_has(BH_KPRIM_CAP_ATOMIC_64) == true);
    assert(bh_kprim_has_any(BH_KPRIM_CAP_ATOMIC_64) == true);
    assert(bh_kprim_has_local(BH_KPRIM_CAP_ATOMIC_64) == true);
    assert(bh_kprim_get_support_level(BH_KPRIM_CAP_ATOMIC_64) == BH_PRIMITIVE_HARDWARE_ASSISTED);

    assert(bh_kprim_has(BH_KPRIM_CAP_TLB_PAGE_INVALIDATE) == true);

    // Unset ones should be false
    assert(bh_kprim_has(BH_KPRIM_CAP_TLB_RANGE_INVALIDATE) == false);
    assert(bh_kprim_has(BH_KPRIM_CAP_IOMMU) == false);
    assert(bh_kprim_get_support_level(BH_KPRIM_CAP_IOMMU) == BH_PRIMITIVE_UNSUPPORTED);

    // Invalid ID should return false
    assert(bh_kprim_has((bh_kprim_capability_t)999) == false);
}

int main(void) {
    printf("Running KPRIM capability tests...\n");
    test_initialization();
    test_queries();
    printf("All tests passed.\n");
    return 0;
}
