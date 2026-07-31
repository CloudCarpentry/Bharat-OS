#include <standard/errno.h>
#include <standard/assert.h>
#include <bharat/libc/status.h>

int main(void) {
    /* Test initial errno value */
    assert(errno == 0);

    /* Test setting and reading errno */
    errno = EINVAL;
    assert(errno == EINVAL);

    errno = ENOMEM;
    assert(errno == ENOMEM);

    /* Test bh_status_to_errno mapping */
    assert(bh_status_to_errno(BHARAT_STATUS_OK) == 0);
    assert(bh_status_to_errno(BHARAT_STATUS_ERR_INVALID_ARG) == EINVAL);
    assert(bh_status_to_errno(BHARAT_STATUS_ERR_NO_MEMORY) == ENOMEM);
    assert(bh_status_to_errno(BHARAT_STATUS_ERR_UNSUPPORTED) == ENOSYS);
    assert(bh_status_to_errno(BHARAT_STATUS_ERR_BUSY) == EBUSY);
    assert(bh_status_to_errno(BHARAT_STATUS_ERR_TIMEOUT) == ETIMEDOUT);
    assert(bh_status_to_errno(BHARAT_STATUS_ERR_OUT_OF_BOUNDS) == EFAULT);

    /* Test direct negative sys_errno */
    assert(bh_status_to_errno(-2) == 2); /* -SYS_ENOENT -> ENOENT */
    assert(bh_status_to_errno(-22) == 22); /* -SYS_EINVAL -> EINVAL */

    return 0;
}
