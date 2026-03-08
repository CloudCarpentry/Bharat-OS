# Linux Personality Architecture

## Introduction
The Bharat-OS core kernel is designed as a distributed, personality-neutral multikernel. To support binary compatibility with Linux, we are introducing a **Linux Personality Subsystem**. This layer is implemented as a pluggable compatibility layer rather than a kernel-wide assumption. The core kernel remains agnostic of POSIX/Linux semantics, with the subsystem translating Linux syscalls and ABI expectations into Bharat-OS primitives.

## Core Process & Thread Model
The core kernel separates the scheduler thread/task abstraction from the process/address-space abstraction.
To support multi-personality execution, we introduce a `personality_type` field to the `kthread_t` (and/or `kprocess_t`) structure.

- `PERSONALITY_NATIVE`: Threads executing native Bharat-OS ABI.
- `PERSONALITY_LINUX`: Threads executing the Linux ABI.

### Syscall Dispatch
The core `trap_handle` and `syscall_dispatch` routines inspect the current thread's personality. If a thread is `PERSONALITY_LINUX`, its syscalls are routed to a separate Linux syscall table (`linux_syscall_handler`) with Linux ABI argument/return conventions, leaving the native dispatch table untainted.

## VFS and File Descriptor Abstraction
Linux userspace expects a file descriptor (FD) model ("everything is a file"). The Linux personality subsystem introduces a per-process FD table abstraction.
- FDs are mapped to Linux-style file objects, not raw Bharat-OS IPC endpoints.
- These file objects can be backed internally by generic kernel objects (console/tty, pipe, socket, event object, shared memory, IPC channel).
- Standard streams (stdin, stdout, stderr) are attached to a tty/console object at process creation.
- The FD layer handles reference counting, open flags, `cloexec`, `dup` semantics, and `poll` readiness hooks.

## Memory Management (mmap)
The Linux `mmap` syscall expects region-level semantics (VMAs), protection changes, splitting/merging, and future copy-on-write behavior.
- The personality subsystem implements a Linux virtual memory compatibility layer using process VM regions.
- `mmap` operates on these VM regions, not directly on individual pages via `vmm_map_page`. This allows for lazy mapping, anonymous mappings (initially), and file-backed mappings in the future.

## Signals and Async Notifications
The core kernel provides a generic async notification model. The Linux personality translates this into:
- Pending signals
- Blocked masks
- Delivery points on return to userspace
- Signal frame setup

## Synchronization (Futex)
The core kernel provides efficient wait/wake primitives (local fast path, scalable wait queues, cross-core aware wakeups). The Linux `futex` is built on top of this core primitive, avoiding ad-hoc hacks.

## Distributed IPC Integration
Because Bharat-OS is a distributed multikernel:
- Linux personality syscalls are kept local where possible.
- Message passing is used only when crossing core/node ownership boundaries.
- Wakeups, signals, and FD readiness support cross-core delivery.
- The Linux personality consumes the kernel's distributed IPC services but does not redefine them.

## ELF Loader and Userspace ABI
A Linux ELF loader is required for:
- ELF64 support
- auxv, argv/envp stack setup
- Statically linked binaries initially, progressing to dynamic linking (interpreter path, TLS hooks).
