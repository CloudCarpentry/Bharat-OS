# Scheduler Algorithms

## Overview
Because Bharat-OS is built for diverse hardware and use cases, the kernel scheduler implements multiple algorithms tailored to the execution profile of the thread (`kthread_t`).

## General Purpose (GP): CFS-Like
For general-purpose workloads (e.g., UI, background networking), the scheduler uses a Completely Fair Scheduler (CFS) approach based on `vruntime` (virtual runtime).

-   **Mechanism:** Threads accumulate `vruntime` as they execute. The thread with the smallest `vruntime` in the ready queue is chosen to run next.
-   **Priority Weighting:** The physical time a thread is allowed to run before its `vruntime` increases by 1 unit depends on its assigned priority weight. High-priority threads accumulate `vruntime` slower, giving them a larger share of the CPU over time.
-   **Data Structure:** A Red-Black tree or an optimized sorted array (depending on the memory profile) is used to track the ready threads, ensuring `O(log N)` insertion and `O(1)` retrieval of the lowest `vruntime` thread.

## Hard Real-Time (RT): Priority & Deadline (EDF)
For threads marked with the `BHARAT_KERNEL_PROFILE_RT` execution profile (e.g., motor control, sensor fusion), the GP scheduler is bypassed entirely.

-   **Mechanism:** RT threads use a strict static priority queue. The highest-priority ready RT thread *always* preempts any GP thread and any lower-priority RT thread immediately.
-   **EDF (Earliest Deadline First):** Within the same RT priority level, the scheduler can optionally order threads based on a strict deadline (if provided during task creation). The thread with the closest deadline runs first.
-   **Data Structure:** An array of linked lists (`O(1)` scheduler) is used. There is one list per RT priority level (e.g., 0-99). The scheduler simply finds the highest non-empty list and takes the first thread.

## AI Scheduler Integration (AI-MLFQ)
Bharat-OS includes an innovative control-plane hook allowing user-space AI governors to influence the GP scheduling algorithm dynamically.

-   **Telemetry Input:** The kernel gathers hardware performance counters (cycles, instructions, CPI, cache misses) via `ai_sched_update_telemetry()` on context switches.
-   **Suggestions:** The AI governor (running as an isolated user-space service) analyzes this data and sends capability-secured IPC messages (`ai_kernel_ingest_suggestion_ipc()`) to the kernel.
-   **Actions:** These suggestions can request the kernel to:
    -   `AI_ACTION_MIGRATE_TASK`: Move a thread to a different CPU core (e.g., a "big" or "LITTLE" core).
    -   `AI_ACTION_ADJUST_PRIORITY`: Temporarily boost or lower a GP thread's weight.
-   **Bounded Execution:** The kernel evaluates these suggestions securely. The scheduler consumes queued suggestions during the timer tick or scheduling path, not directly in the IPC receive path, ensuring the AI governor cannot cause denial-of-service within the kernel scheduler itself.