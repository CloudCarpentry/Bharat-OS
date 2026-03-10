void linux_vfs_init(void);
#include "linux_compat.h"

#include <stddef.h>

static subsys_instance_t* g_linux_instance;

int linux_subsys_init(subsys_instance_t* env) {
    if (!env) {
        return -1;
    }

    if (env->type != SUBSYS_TYPE_LINUX) {
        return -2;
    }

    env->memory_limit_mb = (env->memory_limit_mb == 0U) ? 1024U : env->memory_limit_mb;
    if (env->cpu_core_allocation_mask == 0U) {
        env->cpu_core_allocation_mask = 0x1U;
    }

    g_linux_instance = env;
    linux_vfs_init();
    return 0;
}

// Placeholder mapping table for FDs
static linux_fd_map_t fd_table[LINUX_MAX_FDS];
static int next_fd = 3; // 0, 1, 2 reserved

int linux_map_fd_to_capability(subsys_instance_t* env, int linux_fd, uint32_t cap, linux_fd_type_t type) {
    if (!env || linux_fd < 0 || linux_fd >= LINUX_MAX_FDS) return -1;
    fd_table[linux_fd].type = type;
    fd_table[linux_fd].linux_fd = linux_fd;
    fd_table[linux_fd].backing_capability = cap;
    fd_table[linux_fd].open_flags = 0;
    fd_table[linux_fd].file_offset = 0;
    fd_table[linux_fd].ref_count = 1;
    return 0;
}

void linux_vfs_init(void) {
    for (int i = 0; i < LINUX_MAX_FDS; i++) {
        fd_table[i].type = LINUX_FD_TYPE_NONE;
        fd_table[i].ref_count = 0;
    }
    // Setup standard streams
    linux_map_fd_to_capability(g_linux_instance, 0, 0, LINUX_FD_TYPE_CONSOLE);
    linux_map_fd_to_capability(g_linux_instance, 1, 0, LINUX_FD_TYPE_CONSOLE);
    linux_map_fd_to_capability(g_linux_instance, 2, 0, LINUX_FD_TYPE_CONSOLE);
}


int linux_syscall_handler(long sysno, long arg1, long arg2, long arg3, long arg4, long arg5, long arg6) {
    (void)arg1;
    (void)arg2;
    (void)arg3;
    (void)arg4;
    (void)arg5;
    (void)arg6;

    if (!g_linux_instance || !g_linux_instance->is_running) {
        return -38; /* ENOSYS-style while subsystem inactive. */
    }

    switch (sysno) {
case 0: /* read */
            if (arg1 >= 0 && arg1 < LINUX_MAX_FDS && fd_table[arg1].type != LINUX_FD_TYPE_NONE) {
                if (fd_table[arg1].type == LINUX_FD_TYPE_CONSOLE) {
                    // map to console read
                    return 0; // return 0 bytes read for now
                }
                return 0; // stub
            }
            return -9; // EBADF
        case 1: /* write */
            if (arg1 >= 0 && arg1 < LINUX_MAX_FDS && fd_table[arg1].type != LINUX_FD_TYPE_NONE) {
                if (fd_table[arg1].type == LINUX_FD_TYPE_CONSOLE) {
                    // For a console, mapping to underlying printing capability
                    return arg3; // return length written as stub success
                }
                return arg3; // stub success
            }
            return -9; // EBADF
        case 2: /* open */
        case 257: /* openat */
            // Allocate an FD, lookup VFS capability
            {
                int fd = next_fd++;
                if (fd >= LINUX_MAX_FDS) return -24; // EMFILE
                linux_map_fd_to_capability(g_linux_instance, fd, 0, LINUX_FD_TYPE_FILE);
                return fd;
            }
        case 3: /* close */
            if (arg1 >= 0 && arg1 < LINUX_MAX_FDS && fd_table[arg1].type != LINUX_FD_TYPE_NONE) {
                fd_table[arg1].ref_count--;
                if (fd_table[arg1].ref_count <= 0) {
                    fd_table[arg1].type = LINUX_FD_TYPE_NONE;
                    fd_table[arg1].linux_fd = -1;
                    fd_table[arg1].backing_capability = 0;
                }
                return 0;
            }
            return -9; // EBADF
        case 9: /* mmap */
            // Map to Linux virtual memory regions instead of raw vmm_map_page
            // For now, return a dummy successful region allocation
            return 0x7F0000000000;
        case 10: /* mprotect */
            return 0; // stub success
        case 11: /* munmap */
            return 0; // stub success
        case 12: /* brk */
            // Return dummy successful heap base
            return 0x40000000;
        case 13: /* rt_sigaction */
            return 0; // stub success
        case 14: /* rt_sigprocmask */
            return 0; // stub success
        case 16: /* ioctl */
            if (arg1 >= 0 && arg1 < LINUX_MAX_FDS && fd_table[arg1].type == LINUX_FD_TYPE_CONSOLE) {
                return 0; // stub success for TTY ioctls
            }
            return -25; // ENOTTY
        case 39: /* getpid */
            return 1;
        case 57: /* fork */
            // Return -ENOSYS as full process duplication isn't ready
            return -38;
        case 59: /* execve */
            // Partial support for starting a new binary. For now just ENOSYS or dummy success
            return -38;
        case 60: /* exit */
        case 231: /* exit_group */
            g_linux_instance->is_running = 0;
            return 0;
        case 63: /* uname */
            return 0; // stub success
        case 186: /* gettid */
            return 1;
        case 202: /* futex */
            return 0; // stub success
        case 228: /* clock_gettime */
            return 0; // stub success
        case 230: /* nanosleep */
            return 0; // stub success
        default:
            return -38; // ENOSYS

    }
}
