---
title: Linux + Android Production-Grade Personality Plan
status: active
owner: Compatibility Team
reviewers: ["Kernel Team", "Perf Team", "CI/Infra Team"]
version: 1.0
last_updated: "2026-04-22"
tags: ["architecture", "personalities", "linux", "android", "production", "performance"]
---

# Linux + Android Production-Grade Personality Plan

## Why this plan exists

We already have architecture documents and high-level roadmaps. This plan converts them into a **single execution backlog** focused on shipping Linux and Android personalities to production quality with a strict **no translation tax** rule.

## Non-negotiable principle: no translation tax

For Linux and Android personalities:

1. Translate ABI only at ingress/egress boundaries.
2. Never perform repeated per-hop translation in hot loops.
3. Reuse native kernel primitives (scheduler, VMM, IPC, capability model).
4. Block release if hot-path KPI budgets regress against baseline.

## Task inventory (what to implement)

### A) Linux personality production tasks

1. **Hot syscall fast path completeness**
   - Implement/validate Tier-1 syscall set (`clone`, `futex`, `epoll_*`, `mmap*`, `openat/read/write/close`).
   - Enforce table-driven dispatch and constant-time argument decode.
2. **Per-process personality context caching**
   - Cache ABI mode, TLS semantics, and FD mapping metadata in thread/process descriptors.
   - Remove dynamic string/object lookups from syscall hot path.
3. **Signal and restart semantics hardening**
   - Ensure EINTR/restart behavior matches Linux userland expectations.
   - Add conformance tests across x86_64, arm64, riscv64.
4. **Perf counter instrumentation**
   - Add counters for fallback-path hits, translation-cache misses, and extra copy events.

### B) Android personality production tasks

1. **Linux baseline reuse (no duplication)**
   - Build Android path explicitly as Linux baseline + Android delta.
   - Ban duplicated Linux syscall implementations under Android personality.
2. **Binder fast-path quality**
   - Fixed decode tables for common commands.
   - Capability transfer with zero additional memory copies on hot path.
   - Binder threadpool wake latency budget enforcement.
3. **ashmem/memfd compatibility hardening**
   - Prefer memfd-backed semantics where possible.
   - Ensure map/protect/unmap paths route through native VMM primitives without side translation.
4. **Android service smoke flows**
   - Property service, binder transact/reply, and shared-memory producer/consumer tests on all ISAs.

### C) Shared platform tasks (Linux + Android)

1. **Cross-ISA ABI parity**
   - Validate syscall frame contracts and errno mapping on x86_64, arm64, riscv64.
2. **Zero-translation observability**
   - Expose tracing labels for translation at boundary, cache hit/miss, and fallback events.
3. **CI release gates**
   - Per-ISA build + smoke + perf jobs are required checks.
   - Release blocked if any single ISA violates hot-path budget.
4. **Regression policy**
   - Any Tier-3 emulation path for common operations must include owner, ticket, KPI impact, and removal milestone.

## Implementation sequence (how to implement)

### Phase 0 — Baseline freeze (Week 0-1)

- Lock benchmark harness and baseline environment for Linux native comparison.
- Finalize KPI dashboards and pass/fail threshold wiring.
- Freeze syscall map generation inputs per ISA.

### Phase 1 — Linux hot-path completion (Week 1-4)

- Complete missing Tier-1 syscalls and remove dynamic lookup from hot path.
- Merge syscall-map drift checker into CI.
- Run tri-ISA busybox + syscall microtests.

### Phase 2 — Android delta hardening (Week 3-7)

- Enforce Android = Linux + delta layering in code ownership and CI checks.
- Optimize binder transfer path and add explicit copy-count assertions.
- Validate ashmem/memfd flows under pressure/load tests.

### Phase 3 — KPI closure and release readiness (Week 6-10)

- Pass all Tier-2 latency/throughput budgets per ISA.
- Publish limitation matrix with explicit mitigation ETA for any waived item.
- Promote to release candidate only after zero-translation counters confirm boundary-only behavior.

## Done criteria per subsystem

A subsystem is production-ready only when all are true:

1. Correctness tests pass on x86_64, arm64, and riscv64.
2. No repeated translation in hot path (verified by traces/counters).
3. Perf budgets are green vs baseline.
4. Fallback paths are either removed or explicitly waived with owner + deadline.

## Immediate execution board (next implementation tranche)

- [x] Add Linux/Android shared translation-event tracepoints (boundary enter/exit, cache miss, fallback).
- [ ] Add syscall-map drift CI check for all supported ISAs.
- [x] Complete Tier-1 Linux syscall parity matrix (Smoke test subset: open, read, write, close, futex) and mark unsupported syscalls explicitly.
- [x] Add binder copy-count assertion tests and p50/p99 latency export.
- [x] Add tri-ISA smoke target that runs Linux busybox and Android service loops in one pipeline.
- [ ] Add release-blocking perf rule: fail if any hot-path delta exceeds contract.

## What was implemented by this document update

This change implements the missing execution layer in documentation by:

1. Translating architecture principles into a concrete Linux + Android task inventory.
2. Defining an implementation sequence with phased ownership outcomes.
3. Adding explicit done criteria and an immediate execution board tied to no-translation-tax enforcement.
