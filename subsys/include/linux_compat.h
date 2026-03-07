#ifndef BHARAT_LINUX_COMPAT_H
#define BHARAT_LINUX_COMPAT_H

#include "subsys.h"
#include "../../kernel/include/capability.h"

/*
 * Bharat-OS Linux Compatibility Layer
 * Requirements Mapping:
 * - Emulates Linux Kernel ABI (Syscalls, /proc, /sys filesystem representations).
 * - Maps Linux ELF loading to native memory management.
 * - Leverages Bharat-OS Security by operating inside a strict Capability Sandbox.
 */

/*
 * Maps a Linux File Descriptor to a native Bharat-OS Capability.
 * This allows high-speed RPC and URPC backend processing for things like
 * epoll and socket communication without ambient permissions.
 */
typedef struct {
    int linux_fd;
    uint32_t backing_capability; // Typically points to a VFS node or Endpoint
} linux_fd_map_t;

int linux_subsys_init(subsys_instance_t* env);

/*
 * Primary entry point for trapped Linux syscalls (e.g., from int 0x80 or syscall instr).
 */
int linux_syscall_handler(long sysno, long arg1, long arg2, long arg3, long arg4, long arg5, long arg6);

/*
 * Register a file descriptor mapping to route Linux I/O to a specific capability backend.
 */
int linux_map_fd_to_capability(subsys_instance_t* env, int linux_fd, uint32_t cap);

#endif // BHARAT_LINUX_COMPAT_H
