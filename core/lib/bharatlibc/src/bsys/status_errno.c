#include <bharat/libc/status.h>
#include <standard/errno.h>

int bh_status_to_errno(int32_t status) {
    /* If it's a negative system errno (from a direct system call return), convert it to positive */
    if (status < 0 && status > -200) {
        return -status;
    }

    switch (status) {
        case BHARAT_STATUS_OK:
            return 0;
        case BHARAT_STATUS_ERR_INVALID_ARG:
            return EINVAL;
        case BHARAT_STATUS_ERR_NO_MEMORY:
            return ENOMEM;
        case BHARAT_STATUS_ERR_UNSUPPORTED:
            return ENOSYS;
        case BHARAT_STATUS_ERR_BUSY:
            return EBUSY;
        case BHARAT_STATUS_ERR_TIMEOUT:
            return ETIMEDOUT;
        case BHARAT_STATUS_ERR_OUT_OF_BOUNDS:
            return EFAULT;
        default:
            return EINVAL;
    }
}
