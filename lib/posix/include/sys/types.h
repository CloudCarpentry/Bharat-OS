#ifndef BHARAT_POSIX_SYS_TYPES_H
#define BHARAT_POSIX_SYS_TYPES_H

#include <stdint.h>

/*
 * Freestanding-friendly POSIX scalar type aliases.
 * These guards avoid duplicate typedefs across libc/toolchain headers.
 */
#ifndef BHARAT_POSIX_SSIZE_T_DEFINED
#define BHARAT_POSIX_SSIZE_T_DEFINED 1
typedef intptr_t ssize_t;
#endif

#ifndef BHARAT_POSIX_PID_T_DEFINED
#define BHARAT_POSIX_PID_T_DEFINED 1
typedef int32_t pid_t;
#endif

#ifndef BHARAT_POSIX_OFF_T_DEFINED
#define BHARAT_POSIX_OFF_T_DEFINED 1
typedef int64_t off_t;
#endif

#ifndef BHARAT_POSIX_UID_T_DEFINED
#define BHARAT_POSIX_UID_T_DEFINED 1
typedef uint32_t uid_t;
#endif

#ifndef BHARAT_POSIX_GID_T_DEFINED
#define BHARAT_POSIX_GID_T_DEFINED 1
typedef uint32_t gid_t;
#endif

#endif /* BHARAT_POSIX_SYS_TYPES_H */
