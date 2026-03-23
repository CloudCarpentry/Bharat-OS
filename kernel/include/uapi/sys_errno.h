#ifndef BHARAT_UAPI_SYS_ERRNO_H
#define BHARAT_UAPI_SYS_ERRNO_H

#include <stdint.h>

/*
 * Bharat-OS Syscall ABI Errors
 * A very stable, narrow set of POSIX-like errors exposed to userland via syscalls.
 * Internal kstatus_t must be mapped to one of these values before crossing the boundary.
 */

typedef int32_t sys_errno_t;

/* Positive numbers match standard POSIX errno, returned negatively from syscalls */
#define SYS_EPERM       1  /* Operation not permitted */
#define SYS_ENOENT      2  /* No such file or directory */
#define SYS_ESRCH       3  /* No such process */
#define SYS_EINTR       4  /* Interrupted system call */
#define SYS_EIO         5  /* I/O error */
#define SYS_EBADF       9  /* Bad file number */
#define SYS_EAGAIN      11 /* Try again */
#define SYS_ENOMEM      12 /* Out of memory */
#define SYS_EACCES      13 /* Permission denied */
#define SYS_EFAULT      14 /* Bad address */
#define SYS_EBUSY       16 /* Device or resource busy */
#define SYS_EEXIST      17 /* File exists */
#define SYS_ENODEV      19 /* No such device */
#define SYS_ENOTDIR     20 /* Not a directory */
#define SYS_EISDIR      21 /* Is a directory */
#define SYS_EINVAL      22 /* Invalid argument */
#define SYS_ENOSPC      28 /* No space left on device */
#define SYS_EROFS       30 /* Read-only file system */
#define SYS_EPIPE       32 /* Broken pipe */
#define SYS_ENOSYS      38 /* Function not implemented */
#define SYS_ENXIO       6  /* No such device or address */
#define SYS_EADDRNOTAVAIL 99 /* Cannot assign requested address */
#define SYS_ENETDOWN    100 /* Network is down */
#define SYS_ENETUNREACH 101 /* Network is unreachable */
#define SYS_ECONNRESET  104 /* Connection reset by peer */
#define SYS_ETIMEDOUT   110 /* Connection timed out */
#define SYS_ECONNREFUSED 111 /* Connection refused */
#define SYS_EHOSTUNREACH 113 /* No route to host */

#endif // BHARAT_UAPI_SYS_ERRNO_H
