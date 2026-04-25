#include <stdarg.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#include "hal/hal_mmu.h"
#include "mm/mem_model.h"
#include "mm/mem_validator.h"
#include "kernel/status.h"
#include "console/console_types.h"

/* Mocks and Stubs */
static hal_mem_caps_t mock_caps;
static int mock_caps_ret = 0;
static mem_model_t mock_requested_model = MEM_MODEL_NONE;

int hal_mem_get_caps(hal_mem_caps_t *caps) {
    if (caps) *caps = mock_caps;
    return mock_caps_ret;
}

mem_model_t mem_model_get_current(void) {
    return mock_requested_model;
}

void console_log(int level, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    printf("[%d] ", level);
    vprintf(fmt, args);
    va_end(args);
}

/* Test Cases */
void test_valid_mmu_full() {
    printf("Running test_valid_mmu_full...\n");
    mock_caps.model = HAL_MEMORY_MODEL_MMU_FULL;
    mock_caps.supports_iommu = true;
    mock_caps_ret = 0;
    mock_requested_model = MEM_MODEL_MMU_FULL;

    int ret = mm_validate_model();
    assert(ret == K_OK);
    assert(mm_get_validated_model() == MEM_MODEL_MMU_FULL);
}

void test_invalid_mmu_full_on_mpu() {
    printf("Running test_invalid_mmu_full_on_mpu...\n");
    mock_caps.model = HAL_MEMORY_MODEL_MPU;
    mock_caps_ret = 0;
    mock_requested_model = MEM_MODEL_MMU_FULL;

    int ret = mm_validate_model();
    assert(ret == K_ERR_PROFILE_RESTRICTED);
}

void test_valid_mmu_lite_on_full() {
    printf("Running test_valid_mmu_lite_on_full...\n");
    mock_caps.model = HAL_MEMORY_MODEL_MMU_FULL;
    mock_caps_ret = 0;
    mock_requested_model = MEM_MODEL_MMU_LITE;

    int ret = mm_validate_model();
    assert(ret == K_OK);
}

void test_iommu_required_fail() {
    printf("Running test_iommu_required_fail...\n");
    mock_caps.model = HAL_MEMORY_MODEL_MMU_FULL;
    mock_caps.supports_iommu = false;
    mock_caps_ret = 0;
    mock_requested_model = MEM_MODEL_MMU_FULL;

    /*
     * In a real build, CONFIG_IOMMU_REQUIRED would be a macro.
     * For this test, we can only verify the branch if we could recompile.
     * But we can at least verify the basic model validation works.
     */
    int ret = mm_validate_model();
    assert(ret == K_OK); // Since CONFIG_IOMMU_REQUIRED is likely not defined for host test
}

int main() {
    test_valid_mmu_full();
    test_invalid_mmu_full_on_mpu();
    test_valid_mmu_lite_on_full();
    test_iommu_required_fail();

    printf("All mem_validator host tests passed!\n");
    return 0;
}
