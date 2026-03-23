# Real-Time Support

## Overview
Bharat-OS is designed to support mixed-criticality workloads on the same hardware. A drone might run a Linux-compatible General Purpose (GP) payload for video streaming while simultaneously running a Hard Real-Time (RT) flight controller loop.

The scheduler guarantees deterministic execution bounds for RT threads.

## The `BHARAT_KERNEL_PROFILE_RT` Profile
When the kernel or a specific task is configured for the RT profile, the scheduler bypasses the standard fairness algorithms (CFS/MLFQ) entirely.

### Characteristics of RT Threads
1.  **Strict Static Priority:** RT threads have fixed priorities (e.g., 0-99). The scheduler *always* selects the highest-priority runnable RT thread.
2.  **Absolute Preemption:** If an RT thread with priority 90 wakes up, it immediately preempts any running thread with priority < 90, whether that thread is GP or RT. There is no concept of "fairness" or "time slices" preventing this.
3.  **No Dynamic Penalties:** RT threads are never penalized for consuming too much CPU time. A runaway RT thread *will* starve the rest of the system (by design).

## Earliest Deadline First (EDF) Scheduling
Within the same static priority level, Bharat-OS can employ an optional EDF scheduler.

### The EDF Model
-   **Parameters:** When an RT thread is created or configured (e.g., via a specialized `sys_sched_set_deadline` capability), it provides three parameters:
    1.  **Period ($P$):** How often the task must execute (e.g., every 10ms).
    2.  **Deadline ($D$):** When the task must complete its execution relative to the start of the period (often $D \le P$).
    3.  **Worst-Case Execution Time ($C$):** The maximum CPU time the task will consume per period.

-   **Mechanism:** At any given moment, the scheduler looks at all runnable RT threads at the highest priority level. It selects the thread whose absolute deadline (current time + $D$) is closest.

### Why EDF?
-   **Optimality:** EDF is mathematically proven to be an optimal uniprocessor scheduling algorithm. If a set of independent, preemptible tasks *can* be scheduled to meet all deadlines, EDF *will* find a valid schedule.
-   **High Utilization:** Unlike Rate Monotonic Scheduling (RMS), which requires tasks to have priorities inversely proportional to their periods and maxes out at ~69% CPU utilization, EDF can theoretically schedule task sets up to 100% CPU utilization while guaranteeing all deadlines are met.

## Real-Time Constraints & Multikernel

A critical challenge in monolithic real-time kernels is lock contention and unbounded latency caused by other cores (e.g., TLB shootdowns or global spinlocks).

### The Bharat-OS Advantage
Because Bharat-OS is a multikernel, true isolation is possible:

1.  **Dedicated RT Cores:** A system can isolate a subset of cores (e.g., Core 0 and Core 1) exclusively for RT tasks.
2.  **No Global Locks:** The multikernel architecture inherently lacks global spinlocks. A GP thread on Core 3 cannot block an RT thread on Core 0 by holding a kernel lock, because they do not share kernel data structures.
3.  **Asynchronous URPC:** If Core 0 needs to coordinate with Core 3 (e.g., sending sensor data), it uses asynchronous, lockless URPC message passing. The RT thread on Core 0 never blocks waiting for the GP thread on Core 3 to acknowledge the message; it simply places it in the ring buffer and continues its deterministic execution loop.