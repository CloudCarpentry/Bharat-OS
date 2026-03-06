# ADR-005: ML Heuristics Kept Out of Ring-0

## Status

Accepted

## Context

Early architectural iterations proposed "AI-Native OS Scheduling," where Machine Learning Reinforcement Learning models (RL agents) were embedded completely inside the microkernel (Ring-0) to dynamically tune scheduling, paging, and I/O logic.

## Decision

**ML must stay entirely out of Ring-0.**
While the OS supports AI-aware tuning, we architecturally enforce a strict separation of Mechanism vs. Policy:

- **Mechanism (Ring-0)**: The microkernel exports rich hardware telemetry, performance counters, and bounded policy tuning hooks (e.g., setting a CPU timeslice coefficient or page eviction threshold).
- **Policy (Ring-3 User-Space)**: ML heuristic daemons (RL agents) run as unprivileged tasks in user-space. They observe the telemetry and invoke capabilities to adjust the bounded tuning hooks.

## Consequences

### Positive

- **Determinism**: Kernel code must be deterministic, bounded, verifiable, and simple to recover from failure. An AI black box directly inside the kernel scheduler would render the system unpredictable under load and impossible to debug.
- **Security**: The TCB remains extremely small. If the ML daemon crashes or behaves erratically, the kernel simply bounds the tuning knobs to safe defaults or restarts the daemon.
- **Formal Verification**: It preserves the mathematical verifiability of the core kernel.

### Negative

- **Latency**: Passing telemetry data out to Ring-3 and invoking a capability back into Ring-0 imposes minor latency compared to an unsafe, in-line Ring-0 function call.
