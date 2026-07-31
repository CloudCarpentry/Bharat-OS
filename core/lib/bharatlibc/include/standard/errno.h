#ifndef BHARATLIBC_ERRNO_H
#define BHARATLIBC_ERRNO_H

#ifdef __cplusplus
extern "C" {
#endif

extern int *__bh_libc_errno_location(void);
#define errno (*__bh_libc_errno_location())

/* Map standard POSIX error codes to Bharat UAPI values */
#define EPERM           1
#define ENOENT          2
#define ESRCH           3
#define EINTR           4
#define EIO             5
#define ENXIO           6
#define EBADF           9
#define EAGAIN          11
#define ENOMEM          12
#define EACCES          13
#define EFAULT          14
#define EBUSY           16
#define EEXIST          17
#define ENODEV          19
#define ENOTDIR         20
#define EISDIR          21
#define EINVAL          22
#define ENOSPC          28
#define EROFS           30
#define EPIPE           32
#define ENOSYS          38
#define EADDRNOTAVAIL   99
#define ENETDOWN        100
#define ENETUNREACH     101
#define ECONNRESET      104
#define ETIMEDOUT       110
#define ECONNREFUSED    111
#define EHOSTUNREACH    113

#ifdef __cplusplus
}
#endif

#endif /* BHARATLIBC_ERRNO_H */
