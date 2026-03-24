# ADR-001: Microkernel vs Hybrid Architecture

## Status

Accepted

## Context

When designing Bharat-OS from scratch, the primary architectural decision is whether to adopt a Monolithic, Hybrid (like Windows NT), or true Microkernel design. Monolithic kernels (Linux) offer peak theoretical performance due to shared address spaces but suffer from massive attack surfaces and extreme complexity. Hybrid kernels attempt to isolate subsystems but still execute drivers in Ring-0.

## Decision

We chose a **Verification-First Microkernel**.
Inspired by seL4, the Trusted Computing Base (TCB) executes strictly in Ring-0 and provides _only_ three primitives:

1. Thread scheduling
2. Memory mapping (paging)
3. Synchronous IPC messaging

All file systems, network stacks, and hardware device drivers execute as isolated tasks in unprivileged user-space (Ring-3).

## Consequences

### Positive

- **Security & Reliability**: A bug in a network driver cannot crash the entire system. The microkernel can simply restart the crashed driver's task.
- **Formal Verification**: By keeping the kernel under 15,000 lines of C/ASM code, it becomes mathematically feasible to prove the absence of buffer overflows and logic flaws.
- **Modularity**: Subsystems can be swapped out without recompiling the core kernel.

### Negative

- **IPC Overhead**: System calls incur a performance penalty because data must be copied across process boundaries via IPC rather than a simple function call. We mitigate this using lockless URPC messaging for high-volume transactions (Bharat-Cloud).
