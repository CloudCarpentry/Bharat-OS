#include <assert.h>
#include <stdio.h>
#include <bharat/uapi/sys_errno.h>
#include "kernel/status.h"

void test_kstatus_mapping(void) {
    printf("Testing kstatus to sysret mapping...\n");

    // Success case
    assert(kstatus_to_sysret(K_OK) == 0);
    assert(kstatus_to_sysret(123) == 123); // Positive values are success payloads

    // Common error cases
    assert(kstatus_to_sysret(K_ERR_INVALID_ARG) == -SYS_EINVAL);
    assert(kstatus_to_sysret(K_ERR_NOT_FOUND) == -SYS_ENOENT);
    assert(kstatus_to_sysret(K_ERR_NO_MEMORY) == -SYS_ENOMEM);
    assert(kstatus_to_sysret(K_ERR_DENIED) == -SYS_EACCES);
    assert(kstatus_to_sysret(K_ERR_UNSUPPORTED) == -SYS_ENOSYS);
    assert(kstatus_to_sysret(K_ERR_BUSY) == -SYS_EBUSY);
    assert(kstatus_to_sysret(K_ERR_AGAIN) == -SYS_EAGAIN);
    assert(kstatus_to_sysret(K_ERR_FAULT) == -SYS_EFAULT);

    // Unknown error case
    assert(kstatus_to_sysret((kstatus_t)-9999) == -SYS_EIO);

    printf("kstatus mapping tests passed.\n");
}

int main(void) {
    test_kstatus_mapping();
    return 0;
}
