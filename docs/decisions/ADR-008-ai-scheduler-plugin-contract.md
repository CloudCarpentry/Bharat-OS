# ADR-008: AI Scheduler Plugin Contract Across Profiles and Architectures

## Status

Accepted

## Context

AI-guided scheduling in Bharat-OS needs to remain compatible with:

- multiple deployment profiles (`RTOS`, `EDGE`, `DESKTOP`), and
- multiple architectures (`x86_64`, `riscv64`, `arm64`).

A direct, hard-coded implementation of telemetry counters inside scheduler core logic makes the system fragile for profile changes and architectures that do not expose identical PMCs.

## Decision

Define a **pluggable AI telemetry contract** with fallback behavior:

1. Scheduler core only depends on profile/arch-neutral functions in `advanced/ai_sched.h`.
2. Architecture-specific PMC sampling is exposed through `ai_sched_arch_sample_pmc(...)` and can be overridden by arch HAL code.
3. If PMCs are unavailable, scheduler telemetry falls back to deterministic approximation (time-slice and run-queue based estimates).
4. Profile-specific behavior is constrained to bounded compile-time scaling paths (e.g., RTOS/EDGE/DESKTOP fallback cycle factors) rather than policy logic spread across scheduler internals.

## Consequences

### Positive

- Scheduler core remains portable and testable across architectures.
- Profile tuning remains bounded and explicit.
- PMCs become optional accelerators, not hard requirements.
- Better alignment with ADR-005 (ML policy out of ring-0, bounded mechanism in kernel).

### Negative

- Fallback telemetry may be less accurate than hardware PMCs.
- Additional interface maintenance is required between scheduler, AI bridge, and arch HAL overrides.
