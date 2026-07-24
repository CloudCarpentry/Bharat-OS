#include "linux_errno.h"
#include "kernel/status.h"

int linux_errno_from_bh_status(kstatus_t status) {
    if (status >= 0) return 0;

    switch (status) {
        case K_ERR_INVALID_ARG:       return LINUX_EINVAL;
        case K_ERR_NOT_FOUND:         return LINUX_ENOENT;
        case K_ERR_UNSUPPORTED:       return LINUX_ENOSYS;
        case K_ERR_BUSY:              return LINUX_EBUSY;
        case K_ERR_TIMEOUT:           return LINUX_EAGAIN;
        case K_ERR_RETRY:             return LINUX_EAGAIN;
        case K_ERR_AGAIN:             return LINUX_EAGAIN;
        case K_ERR_FAULT:             return LINUX_EFAULT;
        case K_ERR_DENIED:            return LINUX_EACCES;
        case K_ERR_NO_MEMORY:         return LINUX_ENOMEM;
        case K_ERR_CAP_DENIED:        return LINUX_EACCES;
        case K_ERR_CAP_INVALID:       return LINUX_EINVAL;
        case K_ERR_VFS_IS_DIR:        return LINUX_EISDIR;
        case K_ERR_VFS_NOT_DIR:       return LINUX_ENOTDIR;
        case K_ERR_VFS_READ_ONLY:     return LINUX_EROFS;
        case K_ERR_VFS_OUT_OF_SPACE:  return LINUX_ENOSPC;
        case K_ERR_DEV_NO_DEVICE:     return LINUX_ENODEV;
        default:                      return LINUX_EINVAL;
    }
}
