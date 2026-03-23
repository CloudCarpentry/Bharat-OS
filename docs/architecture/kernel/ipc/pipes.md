# Pipes

## Overview
Pipes are a fundamental POSIX IPC mechanism. They provide a unidirectional, byte-stream interface between two processes. In a monolithic kernel, pipes are managed directly in kernel space via an in-memory ring buffer (`struct pipe_inode_info` in Linux).

In the Bharat-OS microkernel architecture, pipes are not implemented in the core kernel. The kernel only provides low-level Synchronous Endpoints and Shared Memory. Pipes are emulated in user space or provided by a dedicated VFS/Pipe Server.

## Implementation Architecture

### Option 1: VFS Server (Standard POSIX Emulation)
When a program calls `pipe(int pipefd[2])`, the C library (libc) makes an RPC call to the Virtual File System (VFS) Server.
1.  The VFS Server allocates a ring buffer in its own memory.
2.  It creates two capability handles: a Read capability and a Write capability.
3.  It returns these capabilities (wrapped as file descriptors) to the client.
4.  Subsequent `read()` and `write()` calls become RPC messages to the VFS server.

**Pros:** Centralized, strict POSIX compliance (e.g., `O_NONBLOCK`, `select`/`poll` integration).
**Cons:** Slower, as every read/write involves a context switch and IPC message to the VFS server.

### Option 2: Zero-Copy User-Space Pipes (High Performance)
For high-bandwidth scenarios between two known tasks, libc can negotiate a shared memory pipe.
1.  Task A requests a pipe to Task B.
2.  Task A allocates an `Untyped` memory page and retypes it as `Shared Memory`.
3.  Task A creates a ring buffer data structure within that shared page.
4.  Task A delegates the `Shared Memory` capability to Task B via a Synchronous Endpoint.
5.  Task A and Task B read/write to the ring buffer directly using atomic operations, using a futex or an Endpoint signal only to wake up the other side when the buffer was empty/full.

**Pros:** Zero-copy, extremely fast (no context switches for data transfer).
**Cons:** Harder to multiplex with `select`/`poll` across arbitrary file descriptors without complex event notification subsystems.

## Named Pipes (FIFOs)
Named pipes (e.g., created with `mkfifo`) exist in the file system namespace. They behave exactly like Option 1, except they are discoverable via a path rather than an anonymous file descriptor inherited across a fork/exec. The VFS Server manages the name-to-capability lookup.