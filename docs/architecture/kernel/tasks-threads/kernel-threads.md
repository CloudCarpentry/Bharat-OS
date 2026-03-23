# Kernel Threads

## Overview
In the multikernel Bharat-OS architecture, most system services run as User Tasks (e.g., VFS, Networking, Device Drivers). However, a minimal set of threads must execute directly within the Kernel's address space.

## Purpose and Characteristics
**Kernel threads (`kthreads`)** run in Ring 0 (or Supervisor Mode) and have full access to kernel structures. They do not have a user-space stack or a user-space address map.

### Why do we need kthreads?
1.  **Idle Loop:** Every CPU core must have at least one thread to execute when no user tasks are ready. The `idle_thread` executes `WFI` (Wait For Interrupt) or `HLT` to save power.
2.  **Deferred Interrupt Handling (Bottom Halves):** While the top-half IRQ handler must be as fast as possible, longer-running operations can be deferred to a dedicated kernel thread (similar to Linux softirqs or workqueues) before returning to user space.
3.  **Memory Management (Swapping/Reclamation):** Kernel daemons that run periodically to reclaim physical pages (`kswapd` style).
4.  **Early Boot:** The kernel initializes the system before any user task exists. This initial execution context is effectively the first kernel thread.

## Naming Convention
To distinguish kernel threads from user threads in debugging output and process lists, kernel threads typically follow a naming convention, often enclosed in brackets or prefixed with `k`:
- `[idle_0]`, `[idle_1]`
- `[kworker]`
- `[kswapd]`

## The `kthread_t` Structure
A kernel thread uses the same `kthread_t` structure as user threads to be managed by the same scheduler. However, its `trap_frame_t` is set up to return to a kernel instruction pointer rather than a user instruction pointer. It also lacks a separate `address_space_t` (it uses the unified kernel page tables).