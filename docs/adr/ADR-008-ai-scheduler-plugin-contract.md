---
title: ADR-008: AI Scheduler Contract Across Profiles and Architectures
status: Accepted
owner: Documentation Working Group
last_updated: 2026-04-25
tags:
  - docs
  - adr
see_also:
  - README.md
---
# ADR-008: AI Scheduler Contract Across Profiles and Architectures

### Contract Status
- **Spec**: ✅ Documented and versioned
- **Implemented**: 🚧 Pending core/kernel/service behavior merge
- **Validated**: ❌ Pending stress/fault-injection tests


## Status

Accepted

## Context

AI-guided scheduling in Bharat-OS must remain compatible with:

- multiple deployment profiles (`RTOS`, `EDGE`, `DESKTOP`), and
- multiple architectures (`x86_64`, `riscv64`, `arm64`, others).

The scheduler must keep deterministic ownership/lifecycle semantics in-kernel while allowing architecture/profile-specific telemetry sources.

## Decision

Adopt a bounded AI-assist contract:

1. Kernel scheduler owns enqueue/dequeue/state transitions and final mutation authority.
2. AI input is represented as bounded suggestions (`ai_suggestion_t`) processed through a pending queue.
3. Telemetry collection uses arch-neutral structures and a weak arch override hook (`ai_sched_arch_sample_pmc(...)`).
4. If PMCs are unavailable, deterministic fallback telemetry estimation is used.
5. Heavy policy/learning loops are not required in kernel hot paths; kernel keeps mechanism, AI provides hints.

## Consequences

### Positive

- Scheduler ownership invariants remain centralized.
- Portable telemetry path with optional architecture specialization.
- Bounded suggestion queue protects scheduler path from unbounded control-plane work.
- Compatible with ADR-005 policy/mechanism split intent.

### Tradeoffs

- Fallback telemetry has lower fidelity than true hardware counters.
- Additional contract-testing is required between governor/control-plane and kernel action handlers.
