#ifndef POSIX_SYSCALL_NUMBERS_H
#define POSIX_SYSCALL_NUMBERS_H

// Defining basic POSIX syscall numbers mapping (generic or mapping to linux base)
#define POSIX_SYS_READ          0
#define POSIX_SYS_WRITE         1
#define POSIX_SYS_OPEN          2
#define POSIX_SYS_CLOSE         3
#define POSIX_SYS_FSTAT         5
#define POSIX_SYS_MMAP          9
#define POSIX_SYS_MUNMAP        11
#define POSIX_SYS_BRK           12
#define POSIX_SYS_GETPID        39
#define POSIX_SYS_EXIT          60
#define POSIX_SYS_EXIT_GROUP    231

#endif // POSIX_SYSCALL_NUMBERS_H
