#include <mm/mem_model.h>
#include <kernel/status.h>
#include <hal/hal_mm.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

void test_mem_model_validation(void) {
    printf("Running test_mem_model_validation...\n");

    hal_memory_caps_t caps;
    memset(&caps, 0, sizeof(caps));

    // Case 1: MMU_FULL requested, HAL supports it
    caps.supports_mmu_full = true;
    caps.supports_user_kernel_split = true;
    caps.supports_page_protection = true;
    caps.supports_execute_disable = true;
    assert(mem_model_validate_hal_caps(MEM_MODEL_MMU_FULL, &caps) == K_OK);

    // Case 2: MMU_FULL requested, HAL lacks it
    caps.supports_mmu_full = false;
    assert(mem_model_validate_hal_caps(MEM_MODEL_MMU_FULL, &caps) == K_ERR_UNSUPPORTED);

    // Case 3: MMU_FULL requested, HAL lacks user/kernel split
    caps.supports_mmu_full = true;
    caps.supports_user_kernel_split = false;
    assert(mem_model_validate_hal_caps(MEM_MODEL_MMU_FULL, &caps) == K_ERR_UNSUPPORTED);

    // Case 4: MPU requested, HAL supports it
    caps.supports_mmu_full = false;
    caps.supports_mpu_only = true;
    assert(mem_model_validate_hal_caps(MEM_MODEL_MPU, &caps) == K_OK);

    // Case 5: MPU requested, HAL lacks it
    caps.supports_mpu_only = false;
    assert(mem_model_validate_hal_caps(MEM_MODEL_MPU, &caps) == K_ERR_UNSUPPORTED);
}

int main(void) {
    test_mem_model_validation();
    printf("All mem_model host tests passed!\n");
    return 0;
}
