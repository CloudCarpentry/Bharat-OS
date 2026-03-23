# Scheduler Roadmap

## Current Status (v1 Baseline)
Based on current code analysis in `kernel/src/sched/` and `kernel/src/ai_sched.c`:
- ✅ **Basic Thread Queueing**: Implemented (`sched_create_thread`, `sched_enqueue`, `sched_dispatch`).
- ✅ **Core Time Accounting**: Basic timer tick and context switch accounting exist.
- ✅ **AI Governor Ingestion**: Implemented a bounded ingestion queue (`ai_kernel_ingest_suggestion_ipc`) for telemetry-driven scheduling hints.
- ✅ **AI Telemetry Tracking**: Implemented `ai_sched_update_telemetry(ctx, cycles, inst)` to record execution counts for CPI analysis.
- 🟡 **CFS/MLFQ Algorithms**: Basic priority queues exist, but full CFS `vruntime` fairness tracking is still being hardened for GP profiles.
- 🟡 **Per-Core Runqueues**: The multikernel architecture mandates per-core structures, which are partially implemented but need robust lockless fast-paths.
- 🔴 **RT/EDF Scheduling**: Hard real-time deadline scheduling is currently deferred; static priorities are honored, but strict deadline admission control is not yet implemented.
- 🔴 **Load Balancing**: Automatic cross-core thread migration (work stealing) is currently deferred in favor of explicit placement or AI Governor suggestions.

## Near-Term Goals (Next 3-6 Months)
1. **Stabilize CFS for GP Profile**: Fully implement and test the Red-Black tree (or equivalent) for `vruntime`-based fair scheduling.
2. **Lockless Per-Core Runqueues**: Ensure that the `sched_dispatch` fast path requires zero cross-core locks or cache-line bouncing.
3. **AI Governor Action Execution**: Fully implement the kernel execution side of `AI_ACTION_MIGRATE_TASK` and `AI_ACTION_ADJUST_PRIORITY` suggestions.

## Long-Term Vision (1+ Years)
1. **Hard Real-Time Profile (EDF)**: Implement strict Earliest Deadline First scheduling for the `BHARAT_KERNEL_PROFILE_RT` configuration, including admission control to guarantee schedulability.
2. **NUMA-Aware Work Stealing**: Implement a decentralized load balancer that understands NUMA topologies and capability table replication costs before migrating threads.
3. **Energy-Aware Scheduling (EAS)**: Integrate thermal and power domain data to prefer packing tasks onto fewer cores (or LITTLE cores) when battery life is prioritized over latency.