#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <bharat/uapi/syscall/bh_syscall_status.h>
#include "kernel/status.h"
#include "trap/syscall_status.h"

void test_status_mapping(void) {
    printf("Testing kstatus to bh_status mapping...\n");

    assert(kstatus_to_bh_status(K_OK) == BH_OK);
    assert(kstatus_to_bh_status(K_ERR_INVALID_ARG) == BH_ERR_INVALID_ARGUMENT);
    assert(kstatus_to_bh_status(K_ERR_UNSUPPORTED) == BH_ERR_NOT_SUPPORTED);
    assert(kstatus_to_bh_status(K_ERR_DENIED) == BH_ERR_ACCESS_DENIED);
    assert(kstatus_to_bh_status(K_ERR_FAULT) == BH_ERR_FAULT);
    assert(kstatus_to_bh_status(K_ERR_NO_MEMORY) == BH_ERR_NO_MEMORY);

    // Capability errors
    assert(kstatus_to_bh_status(K_ERR_CAP_INVALID) == BH_ERR_BAD_CAPABILITY);
    assert(kstatus_to_bh_status(K_ERR_CAP_STALE) == BH_ERR_STALE_CAPABILITY);
    assert(kstatus_to_bh_status(K_ERR_CAP_DENIED) == BH_ERR_INSUFFICIENT_RIGHTS);

    // Internal/Unmapped
    assert(kstatus_to_bh_status(-9999) == BH_ERR_INTERNAL);

    printf("Status mapping tests passed.\n");
}

void test_sysret_convention(void) {
    printf("Testing sysret convention...\n");

    assert(kstatus_to_native_sysret(K_OK) == (long)BH_OK);
    assert(kstatus_to_native_sysret(123) == 123L);
    assert(kstatus_to_native_sysret(K_ERR_INVALID_ARG) == (long)BH_ERR_INVALID_ARGUMENT);
    assert(kstatus_to_native_sysret(K_ERR_FAULT) == (long)BH_ERR_FAULT);

    printf("Sysret convention tests passed.\n");
}

int main(void) {
    test_status_mapping();
    test_sysret_convention();
    return 0;
}
