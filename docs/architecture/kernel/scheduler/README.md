# Scheduler Architecture

## Overview
The `scheduler` cluster details how the Bharat-OS kernel distributes CPU time among executable threads (`kthread_t`). Because Bharat-OS targets devices ranging from hard real-time controllers to multi-core hybrid cloud servers, the scheduler is highly configurable.

## Goals
1.  **Fairness (GP Profile):** Ensure general-purpose tasks receive an equitable share of the CPU based on their weight (priority).
2.  **Real-Time (RT Profile):** Guarantee deterministic, bounded latencies for high-priority tasks, regardless of system load.
3.  **Energy Efficiency (Edge Profile):** Minimize waking idle cores and consolidate work to save battery on embedded devices.
4.  **AI Governor Integration:** Allow an external AI Service to asynchronously suggest load-balancing, migrations, and priority adjustments based on telemetry.

## Related Documents
- [Algorithms](algorithms.md) - Details the CFS-like and MLFQ approaches used for general-purpose threads.
- [Runqueue Design](runqueue.md) - Per-CPU lockless queues and load balancing mechanisms.
- [Priority & Inversion](priority.md) - Static vs dynamic priorities and Priority Inheritance (PI) protocols.
- [Time Accounting](time-accounting.md) - High-resolution timers, ticks, and `vruntime`.
- [CPU Affinity](cpu-affinity.md) - Pinning threads to specific cores and NUMA-aware scheduling.
- [Preemption](preemption.md) - Kernel preemption points, IRQ-safe regions, and voluntary yielding.
- [Real-Time Support](realtime.md) - The RT task model and EDF (Earliest Deadline First) scheduling.
- [Roadmap](roadmap.md) - Current status and future goals for the scheduler.