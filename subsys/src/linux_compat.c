#include "../include/linux_compat.h"

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
    return 0;
}

// Placeholder mapping table for FDs
static linux_fd_map_t fd_table[256];
static int next_fd = 3; // 0, 1, 2 reserved

int linux_map_fd_to_capability(subsys_instance_t* env, int linux_fd, uint32_t cap) {
    if (!env || linux_fd < 0 || linux_fd >= 256) return -1;
    fd_table[linux_fd].linux_fd = linux_fd;
    fd_table[linux_fd].backing_capability = cap;
    return 0;
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
            // Map to underlying IPC/capability read
            return -38; // TODO: tie to IPC endpoint receiving
        case 1: /* write */
            // Map to underlying IPC/capability write
            return arg3; // return length written as stub success
        case 2: /* open */
        case 257: /* openat */
            // Allocate an FD, lookup VFS capability
            {
                int fd = next_fd++;
                if (fd >= 256) return -24; // EMFILE
                linux_map_fd_to_capability(g_linux_instance, fd, 0 /* dummy cap */);
                return fd;
            }
        case 3: /* close */
            if (arg1 >= 0 && arg1 < 256) {
                fd_table[arg1].linux_fd = -1;
                fd_table[arg1].backing_capability = 0;
                return 0;
            }
            return -9; // EBADF
        case 9: /* mmap */
            // Tie into vmm_map_page or mm_create_address_space allocation
            return -38; // stub
        case 12: /* brk */
            // Return dummy successful heap base
            return 0x40000000;
        case 39: /* getpid */
            return 1;
        case 57: /* fork */
            // Return -ENOSYS as full process duplication isn't ready
            return -38;
        case 59: /* execve */
            // Partial support for starting a new binary. For now just ENOSYS or dummy success
            return -38;
        case 60: /* exit */
            g_linux_instance->is_running = 0;
            return 0;
        case 228: /* clock_gettime */
            return 0; // stub success
        default:
            return -38; // ENOSYS
    }
}
