# Shared Memory

## Overview
Shared Memory is the fastest form of Inter-Process Communication because it avoids data copying entirely. Multiple tasks (processes) map the same physical memory frames into their respective virtual address spaces.

## Mechanism in Bharat-OS

Unlike monolithic kernels that manage a global pool of `shmget` identifiers or POSIX `/dev/shm` files, Bharat-OS uses its fundamental **Capability** and **Memory Management** primitives to provide shared memory securely.

### 1. Creation and Delegation
1.  **Allocation:** Task A uses an `Untyped` capability to allocate a physical memory frame (e.g., a 4KB `CAP_TYPE_FRAME`).
2.  **Mapping:** Task A invokes the `MAP` right on that frame capability, inserting it into its own Address Space (VSpace). It can now read and write to that memory.
3.  **Delegation:** Task A wants to share this memory with Task B. It uses an IPC Endpoint to send a copy of the `CAP_TYPE_FRAME` capability to Task B. Task A can optionally attenuate the rights (e.g., granting only `READ` access to Task B).
4.  **Receiving & Mapping:** Task B receives the capability, places it in its CSpace, and invokes its `MAP` right to insert the frame into its own Address Space.

Now, both Task A and Task B have virtual addresses pointing to the exact same physical memory.

### 2. POSIX `shm_open` / `mmap` (The VFS Server)
To support standard POSIX applications that expect named shared memory segments (e.g., `shm_open("/my_shm", O_CREAT)`), Bharat-OS relies on a user-space VFS Server.

1.  A client calls `shm_open("/my_shm")`. This sends an IPC request to the VFS Server.
2.  The VFS Server acts as a registry. If the segment doesn't exist, it allocates physical memory frames and stores the capabilities.
3.  The client then calls `mmap(fd)`. This sends another IPC request to the VFS Server.
4.  The VFS Server replies by delegating the `CAP_TYPE_FRAME` capabilities associated with that named segment to the client.
5.  The client's libc automatically maps those capabilities into the client's Address Space.

### 3. Coherence and Synchronization
Shared memory provides raw data sharing, but no synchronization. If Task A and Task B write to the memory simultaneously, data corruption occurs.

-   **Futexes:** Tasks must use fast user-space mutexes (futex-like primitives) located *within* the shared memory segment itself to synchronize access.
-   **Lockless Queues:** For high-performance streaming (e.g., audio, video, networking), tasks often implement lockless Single-Producer-Single-Consumer (SPSC) ring buffers within the shared memory.
-   **Cache Coherence:** On SMP systems, the hardware cache coherence protocol (e.g., MESI) ensures that writes by Task A on Core 0 are visible to Task B on Core 1. However, tasks must use memory barriers (`__atomic_thread_fence`) to ensure the *ordering* of those writes is correct.