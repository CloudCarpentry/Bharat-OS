#include "kernel/status.h"
#include <bharat/uapi/sys_errno.h>

int kstatus_to_syserr(kstatus_t st) {
    if (st == K_OK) return 0;

    switch (st) {
        /* General */
        case K_ERR_INVALID_ARG:       return -SYS_EINVAL;
        case K_ERR_BAD_STATE:         return -SYS_EIO;
        case K_ERR_NOT_FOUND:         return -SYS_ENOENT;
        case K_ERR_ALREADY_EXISTS:    return -SYS_EEXIST;
        case K_ERR_UNSUPPORTED:       return -SYS_ENOSYS;
        case K_ERR_BUSY:              return -SYS_EBUSY;
        case K_ERR_TIMEOUT:           return -SYS_ETIMEDOUT;
        case K_ERR_RETRY:
        case K_ERR_AGAIN:             return -SYS_EAGAIN;
        case K_ERR_INTERRUPTED:       return -SYS_EINTR;
        case K_ERR_CANCELLED:         return -SYS_EINTR;
        case K_ERR_PARTIAL:           return -SYS_EIO;
        case K_ERR_OVERFLOW:          return -SYS_EINVAL;
        case K_ERR_CHECKSUM:          return -SYS_EIO;
        case K_ERR_FAULT:             return -SYS_EFAULT;
        case K_ERR_DENIED:            return -SYS_EACCES;
        case K_ERR_INTERNAL_BUG:      return -SYS_EIO;

        /* Memory */
        case K_ERR_NO_MEMORY:         return -SYS_ENOMEM;
        case K_ERR_VM_UNMAPPED:       return -SYS_EFAULT;
        case K_ERR_VM_ALREADY_MAPPED: return -SYS_EINVAL;
        case K_ERR_VM_PROT:           return -SYS_EACCES;
        case K_ERR_PMM_EXHAUSTED:     return -SYS_ENOMEM;
        case K_ERR_DMA_BOUNDARY:      return -SYS_EIO;
        case K_ERR_ALIGNMENT:         return -SYS_EINVAL;

        /* Scheduler / Task */
        case K_ERR_NO_TASK:           return -SYS_ESRCH;
        case K_ERR_BAD_THREAD:        return -SYS_ESRCH;
        case K_ERR_NOT_RUNNABLE:      return -SYS_EINVAL;
        case K_ERR_ALREADY_BLOCKED:   return -SYS_EBUSY;
        case K_ERR_WRONG_AFFINITY:    return -SYS_EINVAL;
        case K_ERR_DEADLINE_MISS:     return -SYS_EIO;
        case K_ERR_QUOTA_EXCEEDED:    return -SYS_EPERM;

        /* Capability / Security */
        case K_ERR_CAP_INVALID:       return -SYS_EBADF;
        case K_ERR_CAP_WRONG_TYPE:    return -SYS_EINVAL;
        case K_ERR_CAP_REVOKED:       return -SYS_EPERM;
        case K_ERR_CAP_DENIED:        return -SYS_EPERM;
        case K_ERR_CAP_OWNERSHIP:     return -SYS_EPERM;
        case K_ERR_LABEL_VIOLATION:   return -SYS_EPERM;
        case K_ERR_SANDBOX_VIOLATION: return -SYS_EPERM;

        /* IPC / URPC */
        case K_ERR_IPC_NO_ENDPOINT:   return -SYS_ENOENT;
        case K_ERR_IPC_CLOSED:        return -SYS_EPIPE;
        case K_ERR_IPC_QUEUE_FULL:    return -SYS_EAGAIN;
        case K_ERR_IPC_PEER_DEAD:     return -SYS_EPIPE;
        case K_ERR_IPC_MSG_TOO_LARGE: return -SYS_EINVAL;
        case K_ERR_IPC_REMOTE_UNAVAIL:return -SYS_ENETUNREACH;
        case K_ERR_IPC_ROUTE_FAILED:  return -SYS_EHOSTUNREACH;
        case K_ERR_IPC_DELEGATION:    return -SYS_EPERM;
        case K_ERR_IPC_CROSS_CORE:    return -SYS_EPERM;

        /* VFS */
        case K_ERR_VFS_NOT_MOUNTED:   return -SYS_ENODEV;
        case K_ERR_VFS_READ_ONLY:     return -SYS_EROFS;
        case K_ERR_VFS_NOT_DIR:       return -SYS_ENOTDIR;
        case K_ERR_VFS_IS_DIR:        return -SYS_EISDIR;
        case K_ERR_VFS_OUT_OF_SPACE:  return -SYS_ENOSPC;
        case K_ERR_VFS_CORRUPTED:     return -SYS_EIO;

        /* Device */
        case K_ERR_DEV_NO_DEVICE:     return -SYS_ENODEV;
        case K_ERR_DEV_NOT_READY:     return -SYS_EAGAIN;
        case K_ERR_DEV_OFFLINE:       return -SYS_ENXIO;
        case K_ERR_DEV_DMA_FAIL:      return -SYS_EIO;
        case K_ERR_DEV_MMIO_FAULT:    return -SYS_EFAULT;
        case K_ERR_DEV_RESET_REQD:    return -SYS_EIO;

        /* Network */
        case K_ERR_NET_IF_DOWN:       return -SYS_ENETDOWN;
        case K_ERR_NET_NO_ROUTE:      return -SYS_ENETUNREACH;
        case K_ERR_NET_BAD_ADDR:      return -SYS_EADDRNOTAVAIL;
        case K_ERR_NET_CONN_RESET:    return -SYS_ECONNRESET;
        case K_ERR_NET_REFUSED:       return -SYS_ECONNREFUSED;
        case K_ERR_NET_UNREACHABLE:   return -SYS_EHOSTUNREACH;

        /* Power / Thermal */
        case K_ERR_PWR_SUSPENDED:     return -SYS_EBUSY;
        case K_ERR_PWR_THROTTLED:     return -SYS_EAGAIN;
        case K_ERR_PWR_THERMAL_TRIP:  return -SYS_EIO;
        case K_ERR_PWR_BATT_CRITICAL: return -SYS_EIO;
        case K_ERR_PWR_TRANS_DENIED:  return -SYS_EPERM;

        /* Policy / AI */
        case K_ERR_POLICY_DENIED:     return -SYS_EACCES;
        case K_ERR_PROFILE_RESTRICTED:return -SYS_EPERM;
        case K_ERR_FEATURE_DISABLED:  return -SYS_ENOSYS;
        case K_ERR_AI_DEFERRED:       return -SYS_EAGAIN;
        case K_ERR_AI_THROTTLED:      return -SYS_EBUSY;

        default:                      return -SYS_EIO;
    }
}
