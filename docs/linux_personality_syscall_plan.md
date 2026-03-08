# Linux Personality Syscall Plan

This document outlines the phased implementation plan for adding the Linux Personality subsystem to Bharat-OS. The subsystem acts as a compatibility layer translating Linux syscalls to Bharat-OS internal operations.

## Phase 1: Minimum Viable Linux Personality

Phase 1 focuses on the most fundamental syscalls to support a basic statically linked Linux executable (e.g., simple compute jobs, C standard library basics).

### Process Management
- `exit`, `exit_group`: Terminate the current thread or process and return status.
- `getpid`, `gettid`: Retrieve process and thread identifiers.
- `uname`: Return system identification information.
- `execve`: Partial implementation to execute a new binary from the VFS.
- `clone`: Initially restricted to a Linux-thread-first form. Full `fork` is deferred.

### Memory Management
- `brk`: Extend the program break (heap).
- `mmap`: Establish virtual memory mappings. Initially focused on anonymous mappings using a VM region abstraction, avoiding direct per-page mappings (`vmm_map_page`).
- `munmap`: Unmap virtual memory regions.
- `mprotect`: Change access protections on memory regions.

### VFS and File Descriptors
- `openat`: Open a file or device relative to a directory file descriptor.
- `close`: Close a file descriptor.
- `read`: Read from a file descriptor.
- `write`: Write to a file descriptor.
- `ioctl`: Minimal implementation for TTY interactions.

### Synchronization and Time
- `clock_gettime`: Retrieve current time from specified clocks.
- `nanosleep`: High-resolution sleep.
- `futex`: Minimal but real implementation built atop core kernel wait/wake primitives.

### Signals
- `rt_sigaction`: Examine or change a signal action.
- `rt_sigprocmask`: Examine or change blocked signals.
- Minimal signal delivery hooks on return to userspace.

### Multiplexing
- `poll` / `ppoll`: Wait for some event on a file descriptor.
- `epoll_create1`, `epoll_ctl`, `epoll_wait`: Implement as soon as practical to support modern Linux event loops.

## Phase 2 and Beyond

Phase 2 will introduce more complex POSIX features, relying on mature Phase 1 infrastructure:
- Full `fork`/`vfork` support (handling the multikernel context).
- File-backed `mmap` and page fault demand paging.
- Advanced socket API (TCP/UDP over Bharat-OS networking).
- Dynamic linking support (ELF interpreter path, TLS).
- Comprehensive `procfs` and `sysfs` stubs where necessary.

**Deferred Features:**
- `io_uring`
- Containers, namespaces, cgroups.
- Advanced process tracing (`ptrace`).
