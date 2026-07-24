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

int hal_mem_get_caps(hal_mem_caps_t *caps) {
    if (caps) *caps = mock_caps;
    return mock_caps_ret;
}

/*
 * Note: mem_model_get_current() is provided by mem_model.c
 * and its return value is controlled by BHARAT_PROFILE_* defines.
 */

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
    memset(&mock_caps, 0, sizeof(mock_caps));
    mock_caps.model = HAL_MEMORY_MODEL_MMU_FULL;
    mock_caps.supports_iommu = true;
    mock_caps.supports_write_protect = true;
    mock_caps_ret = 0;

    int ret = mm_validate_model();
    // Default model is MMU_FULL if no PROFILE define is set
    assert(ret == K_OK);
    assert(mm_get_validated_model() == MEM_MODEL_MMU_FULL);
}

void test_invalid_mmu_full_on_mpu() {
    printf("Running test_invalid_mmu_full_on_mpu...\n");
    memset(&mock_caps, 0, sizeof(mock_caps));
    mock_caps.model = HAL_MEMORY_MODEL_MPU;
    mock_caps_ret = 0;

    int ret = mm_validate_model();
    // Default model is MMU_FULL, so it should fail on MPU
    assert(ret == K_ERR_PROFILE_RESTRICTED);
}

void test_valid_mmu_lite_on_full_synthetic() {
    printf("Running test_valid_mmu_lite_on_full_synthetic...\n");
    mem_runtime_caps_t caps = {0};
    caps.supports_mmu_lite = true;
    caps.supports_page_protection = true;
    caps.supports_region_protection = true;
    caps.supports_shared_memory = true;

    mem_profile_contract_t contract = {0};
    contract.model = MEM_MODEL_MMU_LITE;
    contract.require_page_protection = true;
    contract.require_region_protection = true;
    contract.require_shared_memory = true;

    kstatus_t status = mem_runtime_validate_profile(&caps, &contract);
    assert(status == K_OK);
}

void test_mmu_full_requires_page_prot() {
    printf("Running test_mmu_full_requires_page_prot...\n");
    memset(&mock_caps, 0, sizeof(mock_caps));
    mock_caps.model = HAL_MEMORY_MODEL_MMU_FULL;
    mock_caps.supports_write_protect = false; // Missing page protection
    mock_caps_ret = 0;

    int ret = mm_validate_model();
    assert(ret == K_ERR_PROFILE_RESTRICTED);
}

void test_mpu_only_rejects_demand_paging() {
    printf("Running test_mpu_only_rejects_demand_paging...\n");
    mem_runtime_caps_t caps = {0};
    caps.supports_mpu = true;
    caps.supports_region_protection = true;

    mem_profile_contract_t contract = {0};
    contract.model = MEM_MODEL_MPU;
    contract.require_demand_paging = true;

    kstatus_t status = mem_runtime_validate_profile(&caps, &contract);
    assert(status == K_ERR_PROFILE_RESTRICTED);
}

void test_iommu_required_fail_synthetic() {
    printf("Running test_iommu_required_fail_synthetic...\n");
    mem_runtime_caps_t caps = {0};
    caps.supports_mmu = true;
    caps.supports_page_protection = true;
    caps.supports_region_protection = true;
    caps.supports_shared_memory = true;
    caps.supports_iommu = false; // Missing IOMMU

    mem_profile_contract_t contract = {0};
    contract.model = MEM_MODEL_MMU_FULL;
    contract.require_page_protection = true;
    contract.require_iommu = true;

    kstatus_t status = mem_runtime_validate_profile(&caps, &contract);
    assert(status == K_ERR_PROFILE_RESTRICTED);
}

void test_optional_numa_hugepage_accepted() {
    printf("Running test_optional_numa_hugepage_accepted...\n");
    mem_runtime_caps_t caps = {0};
    caps.supports_mmu = true;
    caps.supports_page_protection = true;
    caps.supports_region_protection = true;
    caps.supports_shared_memory = true;
    caps.supports_numa = false;
    caps.supports_hugepage = false;

    mem_profile_contract_t contract = {0};
    contract.model = MEM_MODEL_MMU_FULL;
    contract.require_page_protection = true;
    contract.require_numa = false; // Optional
    contract.require_hugepage = false; // Optional

    kstatus_t status = mem_runtime_validate_profile(&caps, &contract);
    assert(status == K_OK);
}

void test_unknown_caps_fail_closed() {
    printf("Running test_unknown_caps_fail_closed...\n");
    mem_runtime_caps_t caps = {0}; // All false
    mem_profile_contract_t contract = {0};
    contract.model = (mem_model_t)99; // Unknown model

    kstatus_t status = mem_runtime_validate_profile(&caps, &contract);
    assert(status == K_ERR_PROFILE_RESTRICTED);
}

int main() {
    test_valid_mmu_full();
    test_invalid_mmu_full_on_mpu();
    test_valid_mmu_lite_on_full_synthetic();
    test_mmu_full_requires_page_prot();
    test_mpu_only_rejects_demand_paging();
    test_iommu_required_fail_synthetic();
    test_optional_numa_hugepage_accepted();
    test_unknown_caps_fail_closed();

    printf("All mem_validator host tests passed!\n");
    return 0;
}
