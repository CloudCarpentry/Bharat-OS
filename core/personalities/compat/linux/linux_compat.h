/* SPDX-License-Identifier: MIT */
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
    LINUX_FD_TYPE_PIPE_READ,
    LINUX_FD_TYPE_PIPE_WRITE,
    LINUX_FD_TYPE_EVENTFD,
    LINUX_FD_TYPE_TIMERFD,
    LINUX_FD_TYPE_EPOLL,
    LINUX_FD_TYPE_PROC,
    LINUX_FD_TYPE_SOCKET,
    LINUX_FD_TYPE_DEV_NULL,
    LINUX_FD_TYPE_DEV_ZERO,
    LINUX_FD_TYPE_DEV_URANDOM
} linux_fd_type_t;

typedef struct {
    linux_fd_type_t type;
    int linux_fd;
    uint32_t backing_capability;
    uint32_t open_flags;
    uint64_t file_offset;
    int ref_count;
    void *priv_data;
} linux_fd_map_t;

struct bh_process;

typedef struct linux_process_state {
    linux_fd_map_t fd_table[LINUX_MAX_FDS];
    int next_fd;
    uintptr_t brk_start;
    uintptr_t brk_current;
    uint64_t robust_list_head;
    uint64_t clear_child_tid_ptr;
    uint64_t signal_mask;
    uint64_t fs_base;
    uint64_t gs_base;
    char cwd[256];
} linux_process_state_t;

linux_process_state_t *linux_get_or_create_process_state(struct bh_process *proc);
void linux_destroy_process_state(struct bh_process *proc);

int linux_subsys_init(subsys_instance_t* env);
int linux_map_fd_to_capability(subsys_instance_t* env, int linux_fd, uint32_t cap, linux_fd_type_t type);
long linux_syscall_handler(long sysno, long arg1, long arg2, long arg3, long arg4, long arg5, long arg6);

#endif // BHARAT_LINUX_COMPAT_H
