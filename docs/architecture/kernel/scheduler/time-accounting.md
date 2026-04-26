---
title: Time Accounting
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
# Time Accounting

## Overview
The `scheduler` cluster requires precise and efficient time tracking to distribute CPU time fairly among threads (`bh_thread_t`) and accurately enforce timeouts (e.g., `sys_sleep()`, `sys_futex_wait_timeout()`).

## High-Resolution Timers vs. Ticks

Historically, OS schedulers relied on a fixed-frequency hardware timer interrupt (a "tick", e.g., 100Hz or 1000Hz) to preempt threads and update system time.

### The Tickless Kernel (Dynamic Ticks)
Bharat-OS employs a **Tickless Kernel** (or `NOHZ`) design, especially critical for the Edge/Battery profile.
-   **No Idle Ticks:** When a CPU core is idle, the timer interrupt is disabled entirely. The core can enter a deep sleep state (e.g., `WFI` on ARM, `MWAIT` on x86) and stay there indefinitely, saving significant power.
-   **Dynamic Ticks:** When threads are running, the kernel programs the hardware timer to interrupt exactly when the *next* event is due (e.g., a thread's time slice expires, or a specific timeout occurs), rather than firing pointlessly every 1ms.

### Hardware Timers
The kernel relies on architecture-specific high-resolution timers:
-   **x86_64:** Local APIC Timer (TSC-Deadline mode if available) or HPET.
-   **ARM64:** Generic Timer (System Counter).
-   **RISC-V:** Core Local Interruptor (CLINT) timer (`mtimecmp`).

## Time Accounting Mechanisms

### 1. Wall-Clock Time
The kernel maintains a monotonic clock (time since boot) and a real-time clock (RTC, synced via NTP in user space). This is exposed to user tasks via a shared memory page (vDSO-like mechanism) so they can read the time without a system call.

### 2. Thread Execution Time (`vruntime`)
For General Purpose (GP) tasks using the CFS-like scheduler, the kernel tracks how long a thread has been executing on the CPU.

-   **Mechanism:** When a thread starts running, the kernel reads the high-resolution hardware counter (e.g., the TSC on x86_64). When the thread stops running (preempted or blocked), the kernel reads the counter again and calculates the delta.
-   **`vruntime` Update:** This physical time delta is scaled based on the thread's priority weight.
    -   `vruntime += physical_delta * (weight_average / thread_weight)`
    -   A high-priority thread accumulates `vruntime` slower than a low-priority thread for the same amount of physical execution time.
-   **Fairness:** The scheduler always selects the thread in the Ready queue with the lowest `vruntime`. This ensures that all threads get a fair share of the CPU proportional to their priority over the long term.

### 3. AI Governor Telemetry (`cycles`, `inst`)
In addition to time, the kernel integrates tightly with hardware Performance Monitoring Counters (PMCs).

-   **Mechanism:** During a context switch, the kernel reads PMC registers (e.g., `rdpmc` on x86_64) to record the number of CPU cycles executed and instructions retired by the outgoing thread.
-   **Usage:** This data (`ai_sched_update_telemetry()`) is exposed to the user-space AI Governor service, which calculates the thread's Cycles Per Instruction (CPI). A high CPI indicates a memory-bound or cache-thrashing thread, prompting the AI to migrate it or adjust its priority.