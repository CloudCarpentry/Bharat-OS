---
title: Linux Syscall Compatibility Matrix
status: Draft
owner: Documentation Working Group
last_updated: 2026-04-25
tags:
  - docs
  - architecture
  - personalities
see_also:
  - README.md
---
# Linux Syscall Compatibility Matrix

## Status
Accepted - Phase 4 Ecosystem Complete

## Scope
Mapping between Linux syscalls and Bharat-OS kernel primitives.

## Supported Syscall Surface (x86_64 ABI)

| Syscall | NR | Class | Subsystem Backend | Semantics & Behaviors |
| :--- | :--- | :--- | :--- | :--- |
| `read` | 0 | `BH_SYS_CLASS_IO` | VFS / Pipe / Eventfd / Timerfd / Procfs | Bounded usercopy buffer read, returns byte count or -EAGAIN |
| `write` | 1 | `BH_SYS_CLASS_IO` | VFS / Pipe / Eventfd / Console | Buffered ring write, usercopy, returns byte count or -EPIPE |
| `open` / `openat` | 2 / 257 | `BH_SYS_CLASS_IO` | VFS / Procfs / DevNodes | Resolves `/dev/null`, `/dev/zero`, `/dev/urandom`, `/proc/*`, `/dev/ptmx` |
| `close` | 3 | `BH_SYS_CLASS_IO` | Resource Lifecycle | Decrements refcount, tears down pipe/eventfd/timerfd/epoll instances |
| `stat` / `fstat` / `lstat` / `newfstatat` | 4 / 5 / 6 / 262 | `BH_SYS_CLASS_IO` | VFS / Metadata | Synthesizes standard `linux_stat` buffer (mode, dev, ino, size) |
| `poll` / `ppoll` | 7 / 271 | `BH_SYS_CLASS_IO` | I/O Multiplexing | Inspects descriptor readiness (POLLIN/POLLOUT/POLLHUP/POLLERR) |
| `mmap` / `munmap` / `mprotect` | 9 / 11 / 10 | `BH_SYS_CLASS_MEMORY` | VMM | Translates to `vm_map_region`, `vm_unmap_region`, `vm_protect_region` |
| `brk` | 12 | `BH_SYS_CLASS_MEMORY` | VMM | Dynamically expands/contracts process heap region |
| `pipe` / `pipe2` | 22 / 293 | `BH_SYS_CLASS_IPC` | Pipe Ring Buffer | Bounded FIFO (4096B), nonblocking support, EOF/EPIPE detection |
| `dup` / `dup2` / `dup3` | 32 / 33 / 292 | `BH_SYS_CLASS_IO` | FD Table | Clones descriptor map entry, supports `O_CLOEXEC` |
| `getpid` / `gettid` | 39 / 186 | `BH_SYS_CLASS_PROCESS` | Scheduler | Returns active Bharat-OS process/thread IDs |
| `fork` / `vfork` | 57 / 58 | `BH_SYS_CLASS_PROCESS` | Process Manager | Creates child process state, clones open file descriptors |
| `execve` | 59 | `BH_SYS_CLASS_PROCESS` | Process Manager | Reinitializes address space and image execution context |
| `clone` | 56 | `BH_SYS_CLASS_PROCESS` | Process/Thread | Supports `CLONE_THREAD`, `CLONE_VM`, TLS FS-base, and TID pointers |
| `fcntl` | 72 | `BH_SYS_CLASS_IO` | FD Table | Handles `F_DUPFD`, `F_GETFD`, `F_SETFD`, `F_GETFL`, `F_SETFL` |
| `getcwd` / `chdir` | 79 / 80 | `BH_SYS_CLASS_IO` | VFS Path Resolver | Maintains per-process current working directory string |
| `access` / `faccessat` | 21 / 269 | `BH_SYS_CLASS_IO` | VFS / Capabilities | Verifies capability read/write/exec rights on target path |
| `readlink` / `readlinkat` | 89 / 267 | `BH_SYS_CLASS_IO` | VFS / Procfs | Resolves symbolic links (including `/proc/self/exe`) |
| `getrandom` | 318 | `BH_SYS_CLASS_SYSTEM` | HAL Entropy | Hardware PRNG bounded usercopy stream |
| `eventfd` / `eventfd2` | 284 / 290 | `BH_SYS_CLASS_IPC` | Eventfd Counter | 64-bit notification counter, semaphore decrement, `EFD_NONBLOCK` |
| `epoll_create` / `epoll_create1` | 213 / 291 | `BH_SYS_CLASS_IO` | Epoll Demuxer | Creates epoll instance with wait list table |
| `epoll_ctl` / `epoll_wait` / `pwait` | 233 / 232 / 281 | `BH_SYS_CLASS_IO` | Epoll Demuxer | Registers descriptors (ADD/MOD/DEL), queries multi-source readiness |
| `timerfd_create` / `settime` / `gettime` | 283 / 286 / 287 | `BH_SYS_CLASS_IO` | HAL Monotonic Timer | Armed periodic/one-shot timer expiration tick accumulator |
| `futex` | 202 | `BH_SYS_CLASS_IPC` | Futex Buckets | User-space fast-path wait/wake synchronizer buckets |

## Contract
- Linux personality uses `normalize_syscall_return` to map `kstatus_t` to `-errno`.
- Unsupported syscalls return `-ENOSYS`.

## Failure Behavior
Negative `kstatus_t` in range [-1, -1000] are translated via `linux_errno_from_bh_status`.
Violations of capability rights or memory boundaries fail closed with `-EACCES` or `-EFAULT`.
