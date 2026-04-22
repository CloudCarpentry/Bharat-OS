---
title: Multi-Architecture Personality Enablement Roadmap
status: active
owner: Architecture Team
reviewers: ["Core Team", "Perf Team"]
version: 1.0
last_updated: "2026-04-22"
tags: ["architecture", "personalities", "x86_64", "arm64", "riscv64", "linux", "android", "performance"]
---

# Multi-Architecture Personality Enablement Roadmap

## Objective

Enable Linux and Android application support across **x86_64**, **arm64**, and **riscv64** while preserving a **near-zero translation tax** on hot paths.

This roadmap identifies the concrete tasks required to move from architecture-specific bootstrap support to production-grade personality execution.

## Scope and Constraints

- In scope:
  - Linux personality execution (CLI/server workloads).
  - Android personality execution (service/runtime workloads).
  - Shared performance and observability across three target ISAs.
- Out of scope (for this roadmap version):
  - Full GUI stack parity for all Android devices.
  - Legacy 32-bit ABIs (x86, armv7).
- Non-negotiable:
  - Boundary-only translation.
  - Table-driven dispatch on all hot paths.
  - No regressions to native personality fast path.

## Architecture Matrix and Deliverables

| Layer | x86_64 | arm64 | riscv64 |
|---|---|---|---|
| Trap + syscall entry ABI | Stabilize SysV entry and fast return path | Stabilize AArch64 exception entry + SVC path | Stabilize RV64 ECALL entry + return path |
| Linux personality | Syscall number table + ABI argument marshalling | Syscall number table + ABI argument marshalling | Syscall number table + ABI argument marshalling |
| Android personality | Bionic + Binder + ashmem/memfd compatibility | Bionic + Binder + ashmem/memfd compatibility | Bionic + Binder + ashmem/memfd compatibility |
| Toolchain + CI | Build, boot, and microbench jobs | Build, boot, and microbench jobs | Build, boot, and microbench jobs |
| Perf gates | Tier-2 overhead budget enforcement | Tier-2 overhead budget enforcement | Tier-2 overhead budget enforcement |

## Work Breakdown Structure (WBS)

### WBS-1: ISA ABI Foundations (Kernel Entry + Calling Convention)

1. Define per-ISA syscall frame contract:
   - register usage,
   - error return encoding,
   - restartable syscall behavior,
   - signal/interruption return protocol.
2. Ensure trap handlers expose uniform internal syscall context object.
3. Add architecture-specific conformance tests for:
   - argument extraction correctness,
   - errno mapping,
   - restart semantics.

**Exit criteria:** identical syscall contract behavior across x86_64/arm64/riscv64 for core Linux ABI calls.

### WBS-2: Linux Personality Cross-ISA Bring-Up

1. Create/validate syscall number maps for each ISA (Linux ABIs differ per architecture).
2. Implement table-generation or verification tooling to prevent syscall map drift.
3. Prioritize hot syscall set:
   - process/thread (`clone`, `exit_group`, `set_tid_address`),
   - memory (`mmap`, `munmap`, `mprotect`, `madvise`),
   - I/O (`openat`, `read`, `write`, `close`, `ioctl` subset),
   - readiness/sync (`epoll_*`, `futex`, `eventfd`, `timerfd`).
4. Add per-ISA Linux personality smoke suite using static busybox + syscall microtests.

**Exit criteria:** same Linux smoke workload passes across all three ISAs without ISA-specific behavior forks in core logic.

### WBS-3: Android Personality Cross-ISA Bring-Up

1. Validate Bionic ABI assumptions per ISA:
   - TLS model,
   - vdso/vvar hooks (or compatible fallback),
   - signal trampoline expectations.
2. Implement Binder compatibility fast-path rules:
   - fixed-size command decode tables,
   - capability transfer path without extra copies,
   - threadpool wakeup latency budget.
3. Implement ashmem/memfd compatibility on shared memory object primitives.
4. Add Android command-line/service smoke tests per ISA:
   - property service basics,
   - binder transact/reply loop,
   - shared-memory producer/consumer flow.

**Exit criteria:** minimal Android userspace services boot and communicate on all three ISAs with the same personality code paths.

### WBS-4: Zero-Translation Performance Engineering

1. Remove string-based or dynamic lookup from hot path translation.
2. Introduce compact ID/enumeration mapping caches at personality boundary.
3. Enforce one-time mapping policy (ABI ingress only).
4. Add perf counters for translation hits/misses and fallback-path usage.

**Exit criteria:** no repeated translation on syscall, epoll/futex, binder, or shared-memory hot loops.

### WBS-5: CI, Benchmarking, and Release Gates

1. Continuous jobs per ISA for:
   - kernel build,
   - personality unit/integration tests,
   - Linux and Android smoke suites,
   - microbench KPI checks.
2. Benchmark baselines:
   - native Linux on equivalent hardware/QEMU profile,
   - Bharat Linux personality,
   - Bharat Android personality.
3. Release blockers:
   - any ISA with KPI regressions above budget,
   - any personality path forced into Tier-3 emulation for common operations.

**Exit criteria:** all three ISAs pass build + correctness + KPI thresholds.

## Performance SLOs (Tier-2 Path)

Per ISA, the following targets apply to Linux and Android personalities:

- Null syscall overhead delta: **<= 5%** vs baseline.
- `mmap/munmap/mprotect` overhead delta: **<= 8%**.
- `epoll_wait` wakeup overhead delta: **<= 10%** under load.
- Futex wait/wake p99 latency delta: **<= 10%**.
- Binder transact/reply median delta: **<= 10%** (Android path).
- Zero-copy shared memory throughput delta: **<= 5%**.

Any breach requires explicit waiver and mitigation plan before milestone promotion.

## Milestone Timeline

### M0 — Baseline and Tooling (2-4 weeks)
- WBS-1 contracts finalized.
- Syscall-map validation tooling in place.
- Minimal tri-ISA build/test jobs live.

### M1 — Linux Tri-ISA MVP (4-8 weeks)
- WBS-2 hot syscall set complete.
- Static busybox and syscall smoke tests passing on x86_64/arm64/riscv64.
- Initial perf numbers collected.

### M2 — Android Tri-ISA MVP (6-10 weeks)
- WBS-3 compatibility core complete.
- Binder + shared-memory smoke tests passing on all ISAs.
- Android service-level sanity checks operational.

### M3 — Zero-Translation Hardening (4-8 weeks)
- WBS-4 mapping caches and fast-path cleanup merged.
- Tier-2 perf SLOs met on representative hardware.

### M4 — Release Candidate (2-4 weeks)
- WBS-5 release gates green across all ISAs.
- Documentation, profiling reports, and known gaps published.

## Risks and Mitigations

1. **Syscall map divergence across ISAs**
   - Mitigation: generated tables + CI drift checks.
2. **riscv64 toolchain/runtime maturity issues**
   - Mitigation: dedicated nightly jobs, ABI conformance harness, fallback stubs with explicit metrics.
3. **Android Binder performance regressions**
   - Mitigation: zero-copy transfer audits, lock-contention tracing, scheduler intent tuning.
4. **Feature creep into slow emulation paths**
   - Mitigation: enforce Tier-2-first policy and block merges that add hot-path fallbacks.

## Ownership Model

- **Architecture Team:** ABI contracts, dispatch framework, design guardrails.
- **Kernel Team:** trap/syscall entry, VM/IPC/scheduler fast paths.
- **Compatibility Team:** Linux/Android personality implementation.
- **Performance Team:** benchmarks, KPIs, regression triage.
- **CI/Infra Team:** tri-ISA pipelines and reproducible benchmark environments.

## Definition of Done (Program-Level)

The multi-architecture personality program is complete when:

1. Linux and Android personality smoke + integration suites pass on x86_64, arm64, riscv64.
2. Hot-path operations meet Tier-2 SLO budgets on all ISAs.
3. Translation is boundary-only and verified by counters/traces.
4. Operational documentation and known limitation matrix are published and versioned.
