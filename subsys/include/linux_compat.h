#ifndef BHARAT_LINUX_COMPAT_H
#define BHARAT_LINUX_COMPAT_H

#include <stdint.h>
#include "subsys.h"

#define LINUX_MAX_FDS 256

typedef enum {
    LINUX_FD_TYPE_NONE = 0,
    LINUX_FD_TYPE_CONSOLE,
    LINUX_FD_TYPE_FILE,
    LINUX_FD_TYPE_PIPE,
    LINUX_FD_TYPE_SOCKET
} linux_fd_type_t;

typedef struct {
    linux_fd_type_t type;
    int linux_fd;
    uint32_t backing_capability;
    uint32_t open_flags;
    uint64_t file_offset;
    int ref_count;
} linux_fd_map_t;

int linux_subsys_init(subsys_instance_t* env);
int linux_map_fd_to_capability(subsys_instance_t* env, int linux_fd, uint32_t cap, linux_fd_type_t type);
int linux_syscall_handler(long sysno, long arg1, long arg2, long arg3, long arg4, long arg5, long arg6);

#endif // BHARAT_LINUX_COMPAT_H
