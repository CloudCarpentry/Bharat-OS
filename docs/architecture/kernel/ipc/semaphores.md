# Semaphores

## Overview
POSIX Semaphores (`sem_t`) and System V Semaphores (`struct sembuf`) are robust, kernel-managed synchronization primitives that allow multiple threads or processes to coordinate access to shared resources.

## Semaphores vs. Mutexes
Unlike Mutexes, which are strictly owned by the thread that locked them (and must be unlocked by the same thread), Semaphores are counters. Any thread can increment (V) or decrement (P) the counter. A Mutex is a specialized case of a Binary Semaphore (counter = 1), but with ownership.

## Implementation Architecture

In the Bharat-OS microkernel, traditional kernel-space semaphores are replaced by fast user-space synchronization or user-space services, depending on the scope of the semaphore.

### 1. POSIX Unnamed Semaphores (`sem_init`)
Unnamed semaphores (often placed in shared memory between processes or used within a single process) are implemented entirely in user-space using **atomic operations and Futexes**.

-   **Structure:** `sem_t` is essentially an atomic integer (`count`) and a futex wait queue.
-   **Wait (`sem_wait`):** A thread atomically decrements the `count`. If the new value is `< 0`, the thread has "blocked." It then makes a `sys_futex_wait()` system call to the kernel, passing the address of the `count`. The kernel puts the thread to sleep on a wait queue associated with that physical address.
-   **Post (`sem_post`):** A thread atomically increments the `count`. If the previous value was `< 0` (meaning threads are waiting), it makes a `sys_futex_wake()` system call to wake up one of the sleeping threads.
-   **Performance:** If the semaphore is available (count > 0), `sem_wait` requires zero system calls, making it extremely fast.

### 2. POSIX Named Semaphores (`sem_open`) / System V Semaphores
Named semaphores are identifiable by a string (e.g., `/my_semaphore`) and can be used by unrelated processes that do not share memory natively. These require a centralized registry.

-   **The VFS/Semaphore Server:** A user-space daemon (often the Virtual File System or a dedicated IPC Server) manages these.
-   **Open (`sem_open`):** Process A sends an IPC request to the Semaphore Server to create or open `/my_semaphore`. The server allocates a `sem_t` in its own memory or in a shared memory region it manages.
-   **Wait/Post via IPC:** If the semaphore is managed purely in the server's memory, Process A must send an IPC message (e.g., "Wait on /my_semaphore") to the server. The server decrements the count. If it blocks, the server simply holds Process A's reply capability and does not answer until another process sends a "Post" message.
-   **Wait/Post via Shared Memory (Optimization):** The server can allocate the `sem_t` in a shared memory page and delegate the `CAP_TYPE_FRAME` capability to both Process A and Process B. Once mapped, the processes can use the fast user-space futex implementation directly, bypassing the server entirely for `wait` and `post` operations. This provides the speed of unnamed semaphores with the discoverability of named semaphores.