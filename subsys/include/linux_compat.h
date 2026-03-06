#ifndef BHARAT_LINUX_COMPAT_H
#define BHARAT_LINUX_COMPAT_H

#include "subsys.h"

/*
 * Bharat-OS Linux Compatibility Layer
 * Requirements Mapping:
 * - Emulates Linux Kernel ABI (Syscalls, /proc, /sys filesystem representations).
 * - Maps Linux ELF loading to native memory management.
 * 
 * Future implementation:
 * Translate common Linux syscalls (e.g., mmap, open, read, ioctl)
 * to Bharat-OS Microkernel IPC messages.
 */

int linux_subsys_init(subsys_instance_t* env);
int linux_syscall_handler(long sysno, long arg1, long arg2, long arg3, long arg4, long arg5, long arg6);

#endif // BHARAT_LINUX_COMPAT_H
