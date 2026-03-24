#ifndef BHARAT_UAPI_SYS_ERRNO_H
#define BHARAT_UAPI_SYS_ERRNO_H

#include <stdint.h>

typedef int32_t sys_errno_t;

/* Positive errno values; syscall boundary returns them as negative values. */
#define SYS_EPERM       1
#define SYS_ENOENT      2
#define SYS_ESRCH       3
#define SYS_EINTR       4
#define SYS_EIO         5
#define SYS_ENXIO       6
#define SYS_EBADF       9
#define SYS_EAGAIN      11
#define SYS_ENOMEM      12
#define SYS_EACCES      13
#define SYS_EFAULT      14
#define SYS_EBUSY       16
#define SYS_EEXIST      17
#define SYS_ENODEV      19
#define SYS_ENOTDIR     20
#define SYS_EISDIR      21
#define SYS_EINVAL      22
#define SYS_ENOSPC      28
#define SYS_EROFS       30
#define SYS_EPIPE       32
#define SYS_ENOSYS      38
#define SYS_EADDRNOTAVAIL 99
#define SYS_ENETDOWN    100
#define SYS_ENETUNREACH 101
#define SYS_ECONNRESET  104
#define SYS_ETIMEDOUT   110
#define SYS_ECONNREFUSED 111
#define SYS_EHOSTUNREACH 113

#endif /* BHARAT_UAPI_SYS_ERRNO_H */
