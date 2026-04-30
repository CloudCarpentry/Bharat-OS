#ifndef LINUX_ERRNO_H
#define LINUX_ERRNO_H

#include "posix_errno.h"

#define LINUX_EPERM            POSIX_EPERM
#define LINUX_ENOENT           POSIX_ENOENT
#define LINUX_ESRCH            POSIX_ESRCH
#define LINUX_EINTR            POSIX_EINTR
#define LINUX_EIO              POSIX_EIO
#define LINUX_ENXIO            POSIX_ENXIO
#define LINUX_E2BIG            POSIX_E2BIG
#define LINUX_ENOEXEC          POSIX_ENOEXEC
#define LINUX_EBADF            POSIX_EBADF
#define LINUX_ECHILD           POSIX_ECHILD
#define LINUX_EAGAIN           POSIX_EAGAIN
#define LINUX_ENOMEM           POSIX_ENOMEM
#define LINUX_EACCES           POSIX_EACCES
#define LINUX_EFAULT           POSIX_EFAULT
#define LINUX_EBUSY            POSIX_EBUSY
#define LINUX_EEXIST           POSIX_EEXIST
#define LINUX_EXDEV            POSIX_EXDEV
#define LINUX_ENODEV           POSIX_ENODEV
#define LINUX_ENOTDIR          POSIX_ENOTDIR
#define LINUX_EISDIR           POSIX_EISDIR
#define LINUX_EINVAL           POSIX_EINVAL
#define LINUX_ENFILE           POSIX_ENFILE
#define LINUX_EMFILE           POSIX_EMFILE
#define LINUX_ENOTTY           POSIX_ENOTTY
#define LINUX_EFBIG            POSIX_EFBIG
#define LINUX_ENOSPC           POSIX_ENOSPC
#define LINUX_ESPIPE           POSIX_ESPIPE
#define LINUX_EROFS            POSIX_EROFS
#define LINUX_EMLINK           POSIX_EMLINK
#define LINUX_EPIPE            POSIX_EPIPE
#define LINUX_EDOM             POSIX_EDOM
#define LINUX_ERANGE           POSIX_ERANGE
#define LINUX_ENOSYS           POSIX_ENOSYS

#include "kernel/status.h"
int linux_errno_from_bh_status(kstatus_t status);

#endif // LINUX_ERRNO_H
