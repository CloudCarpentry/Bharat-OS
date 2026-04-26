---
title: Preemption
status: Proposed
owner: Documentation Working Group
last_updated: 2026-04-25
tags:
  - docs
  - architecture
  - kernel
see_also:
  - README.md
---
# Preemption

## Overview
In a multi-tasking OS, the scheduler must guarantee that threads (`bh_thread_t`) share the CPU fairly and that critical high-priority tasks run immediately when ready. This involves **preemption**: forcibly removing a running thread from the CPU and replacing it with another.

## Types of Preemption

### 1. Voluntary Yielding
A thread can explicitly give up the CPU when it has finished its work or is waiting for an event.

-   **System Call:** `sys_yield()`. The thread is placed back on the Ready queue, and the scheduler picks a new thread.
-   **Blocking IPC:** If a thread calls `ipc_endpoint_receive()` and no message is ready, the kernel marks the thread as Blocked and schedules another thread.
-   **Mutex Wait:** If a thread calls `sys_futex_wait()` because a fast user-space mutex is locked, it blocks.

### 2. Involuntary Preemption (Timer Interrupt)
A thread can be forcibly removed from the CPU by the kernel if it runs too long.

-   **Mechanism:** The kernel programs a hardware timer to fire an interrupt after a specific interval (the thread's "time slice"). When the interrupt fires, the CPU jumps to the kernel's IRQ handler (`trap_handle`).
-   **Time Accounting:** The kernel updates the thread's execution time (`vruntime`) and determines if another Ready thread now deserves the CPU more.
-   **Context Switch:** If the scheduler decides to switch, the `trap_handle` saves the current thread's state to its `trap_frame_t` and loads the next thread's state before returning.

### 3. Asynchronous Preemption (Hardware Interrupt)
A high-priority thread might be waiting for a hardware event (e.g., a network packet or sensor reading). When the hardware interrupt fires, the CPU enters the kernel.

-   **Mechanism:** The kernel's IRQ handler acknowledges the hardware and signals an IPC Endpoint (or an Event capability) bound to the interrupt.
-   **Wakeup:** This signal transitions the waiting high-priority thread from Blocked to Ready.
-   **Immediate Preemption:** If the newly Ready thread has a higher priority than the thread that was interrupted, the kernel immediately performs a context switch *before* returning to user space. The original thread is preempted.

## Kernel Preemption

The most critical latency metric in a real-time OS is "Kernel Preemption Latency."

-   **The Problem:** What happens if a hardware interrupt fires and wakes a high-priority RT thread while the CPU is *already executing inside the kernel* (e.g., in the middle of a long system call like `vmm_map_page`)? Can the kernel be preempted?
-   **Monolithic vs. Microkernel:** Traditional monolithic kernels are often "non-preemptible" or only partially preemptible (with complex locking). Bharat-OS, being a microkernel, has extremely short system calls. Most kernel execution paths (like IPC or VMM updates) complete in a few hundred instructions.
-   **Bharat-OS Design:** The Bharat-OS kernel is inherently preemptible at almost any point. However, to protect critical, short-lived kernel data structures (like runqueues or capability tables), it uses tiny IRQ-safe spinlocks or disables local interrupts.

### IRQ-Safe Regions
If a CPU core is holding a spinlock (e.g., modifying its runqueue), it *must not* be preempted by a hardware interrupt that might also need that spinlock (causing a deadlock).

-   **Mechanism:** The kernel uses `hal_cpu_disable_interrupts()` before acquiring the lock and `hal_cpu_enable_interrupts()` after releasing it.
-   **Latency Bound:** The duration that interrupts are disabled is the absolute worst-case preemption latency. In Bharat-OS, this is strictly bounded to a few micro-operations. All long-running work (like parsing a complex VFS path or negotiating network handshakes) happens in preemptible user-space tasks.