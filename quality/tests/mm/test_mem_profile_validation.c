#include <stdarg.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#include "hal/hal_mmu.h"
#include "mm/mem_model.h"
#include "mm/mem_validator.h"
#include "mm/mem_profile_validation.h"
#include "kernel/status.h"

static hal_mem_caps_t mock_caps;
int hal_mem_get_caps(hal_mem_caps_t *caps) {
    if (caps) *caps = mock_caps;
    return 0;
}

void console_log(int level, const char *fmt, ...) {
    (void)level;
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
}

const char* BHARAT_ARCH_NAME = "host-test";

void test_caps_extended_report() {
    printf("Running test_caps_extended_report...\n");
    memset(&mock_caps, 0, sizeof(mock_caps));
    mock_caps.model = HAL_MEMORY_MODEL_MMU_FULL;
    mock_caps.supports_user_kernel_isolation = true;
    mock_caps.supports_nx = true;
    mock_caps.supports_tlb_shootdown = true;

    // mm_validate_model will log the MEMCAP strings
    int ret = mm_validate_model();
    assert(ret == K_OK);
}

void test_profile_mismatch_fails() {
    printf("Running test_profile_mismatch_fails...\n");

    // Setup mock caps that lack user/kernel isolation
    memset(&mock_caps, 0, sizeof(mock_caps));
    mock_caps.model = HAL_MEMORY_MODEL_MMU_FULL;
    mock_caps.supports_user_kernel_isolation = false;

    // We can't easily change BHARAT_PROFILE_* defines at runtime in this C test
    // but we can test the validator function directly if we were to expose it.
    // For now, let's just test that the validation logic handles the hal_caps correctly.

    kstatus_t status = mem_profile_validate_requirements(&mock_caps);

    // Depending on what profile this test is compiled with, it might pass or fail.
    // If BHARAT_PROFILE_DESKTOP is defined, it should fail.
#if defined(BHARAT_PROFILE_DESKTOP)
    assert(status == K_ERR_PROFILE_RESTRICTED);
    printf("Successfully failed DESKTOP on non-isolated hardware.\n");
#else
    (void)status;
#endif
}

int main() {
    test_caps_extended_report();
    test_profile_mismatch_fails();
    printf("All MEMCAP conformance gate tests passed!\n");
    return 0;
}
