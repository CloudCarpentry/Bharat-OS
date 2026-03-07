#ifndef BHARAT_POSIX_SYS_TYPES_H
#define BHARAT_POSIX_SYS_TYPES_H

#include <stdint.h>

/*
 * Freestanding-friendly POSIX scalar type aliases.
 * These guards avoid duplicate typedefs across libc/toolchain headers.
 */
#ifndef _SSIZE_T_DEFINED_
#define _SSIZE_T_DEFINED_ 1
typedef intptr_t ssize_t;
#endif

#ifndef _PID_T_DEFINED_
#define _PID_T_DEFINED_ 1
typedef int32_t pid_t;
#endif

#ifndef _OFF_T_DEFINED_
#define _OFF_T_DEFINED_ 1
typedef int64_t off_t;
#endif

#ifndef _UID_T_DEFINED_
#define _UID_T_DEFINED_ 1
typedef uint32_t uid_t;
#endif

#ifndef _GID_T_DEFINED_
#define _GID_T_DEFINED_ 1
typedef uint32_t gid_t;
#endif

#endif /* BHARAT_POSIX_SYS_TYPES_H */
