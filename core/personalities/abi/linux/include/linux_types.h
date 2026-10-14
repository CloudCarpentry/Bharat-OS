/* SPDX-License-Identifier: MIT */
#ifndef BHARAT_LINUX_TYPES_H
#define BHARAT_LINUX_TYPES_H

#include <stdint.h>
#include <stddef.h>

typedef struct {
    int64_t tv_sec;
    int64_t tv_nsec;
} linux_timespec_t;

struct linux_itimerspec {
    linux_timespec_t it_interval;
    linux_timespec_t it_value;
};
typedef struct linux_itimerspec linux_itimerspec_t;

typedef struct {
    uint64_t rlim_cur;
    uint64_t rlim_max;
} linux_rlimit64_t;

/* Open Flags */
#define LINUX_O_RDONLY   00000000
#define LINUX_O_WRONLY   00000001
#define LINUX_O_RDWR     00000002
#define LINUX_O_CREAT    00000100
#define LINUX_O_EXCL     00000200
#define LINUX_O_NOCTTY   00000400
#define LINUX_O_TRUNC    00001000
#define LINUX_O_APPEND   00002000
#define LINUX_O_NONBLOCK 00004000
#define LINUX_O_CLOEXEC  02000000

/* Fcntl Commands */
#define LINUX_F_DUPFD         0
#define LINUX_F_GETFD         1
#define LINUX_F_SETFD         2
#define LINUX_F_GETFL         3
#define LINUX_F_SETFL         4
#define LINUX_F_DUPFD_CLOEXEC 1030

#define LINUX_FD_CLOEXEC 1

/* File Type Modes */
#define LINUX_S_IFMT   00170000
#define LINUX_S_IFSOCK 0140000
#define LINUX_S_IFLNK  0120000
#define LINUX_S_IFREG  0100000
#define LINUX_S_IFBLK  0060000
#define LINUX_S_IFDIR  0040000
#define LINUX_S_IFCHR  0020000
#define LINUX_S_IFIFO  0010000

#endif /* BHARAT_LINUX_TYPES_H */
