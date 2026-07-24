#include <assert.h>
#include <stdio.h>
#include "linux_errno.h"
#include "kernel/status.h"

// Mock the status conversion function if we can't link it easily,
// or just test the mapping logic if we can.
extern int linux_errno_from_bh_status(kstatus_t status);

void test_linux_errno_mapping(void) {
    printf("Testing Linux errno mapping...\n");

    assert(linux_errno_from_bh_status(K_OK) == 0);
    assert(linux_errno_from_bh_status(K_ERR_NOT_FOUND) == LINUX_ENOENT);
    assert(linux_errno_from_bh_status(K_ERR_UNSUPPORTED) == LINUX_ENOSYS);
    assert(linux_errno_from_bh_status(K_ERR_NO_MEMORY) == LINUX_ENOMEM);
    assert(linux_errno_from_bh_status(K_ERR_DENIED) == LINUX_EACCES);
    assert(linux_errno_from_bh_status(K_ERR_INVALID_ARG) == LINUX_EINVAL);
    assert(linux_errno_from_bh_status(K_ERR_FAULT) == LINUX_EFAULT);

    // Default case
    assert(linux_errno_from_bh_status(-9999) == LINUX_EINVAL);

    printf("Linux errno mapping tests passed.\n");
}

int main(void) {
    test_linux_errno_mapping();
    return 0;
}
