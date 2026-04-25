---
title: Bharat-OS Review: Modern Primitives, Risk, and Personality Impact (Linux/Android)
status: Draft
owner: Documentation Working Group
last_updated: 2026-04-25
tags:
  - docs
  - reviews
see_also:
  - README.md
---
# Bharat-OS Review: Modern Primitives, Risk, and Personality Impact (Linux/Android)

**Date:** 2026-04-20
**Scope:** Intent-based execution, memory semantic classes, and fault domains as additive v1 primitives.

## 1) Executive conclusion

These primitives are **architecturally aligned** with Bharat-OS and can be adopted with **low compatibility risk** and **bounded runtime overhead** if introduced as:

1. **Additive UAPI objects + syscalls** (no replacement of existing personality-facing syscalls).
2. **Mechanism in kernel, policy in services** (scheduler/aigov/supervisor decide how much to honor).
3. **Predictable degrade behavior** on profiles that cannot fully enforce semantics.

For Linux/Android personalities, this supports a **zero-emulation-tax default path**: existing app/runtime behavior stays unchanged unless that personality explicitly opts into mapping semantics.

## 2) Primitive-by-primitive value, risk, and overhead

## 2.1 Intent execution (`intent_set/get` in v1)

### How it helps
- Decouples *execution goals* (latency, energy, reliability, isolation) from hard-coded scheduler policy.
- Creates a stable contract for future heterogeneous scheduling without ABI churn.
- Enables profile-aware treatment (RT can enforce more strictly; GP/mobile can treat as hints).

### Key risks
- **Policy ambiguity:** if services and scheduler interpret the same hint differently, behavior drifts.
- **Overfitting risk:** early implementation may try to enforce too much in kernel.
- **Compatibility drift:** adding intent fields directly to legacy thread-create ABI would threaten personality stability.

### Overhead profile (how to keep minimal)
- Store intent metadata on thread/task object; avoid heavy recomputation in hot path.
- Read intent primarily on enqueue/wakeup transitions, not every timer tick.
- Cache interpreted scheduler hints per runqueue class.
- Keep v1 as metadata + lightweight propagation; defer complex AI decisions to services.

**Expected overhead target:** near-noise for legacy workloads (no intent attached), low single-digit scheduler overhead for intent-attached workloads.

## 2.2 Memory semantic classes (`mem_class_t`)

### How it helps
- Replaces allocator ad-hoc flags with a semantic layer useful across RT/mobile/edge/cloud.
- Creates policy hooks for NUMA placement, reclaim behavior, secure handling, DMA suitability, and future tiered memory.
- Enables per-class accounting and observability for debugging and optimization.

### Key risks
- **Semantic inflation:** too many classes too early may produce unclear ownership and inconsistent use.
- **Backend mismatch:** classes that cannot be honored equally across MMU/MPU/IOMMU profiles.
- **Hidden regressions:** if class validation is strict too early, existing allocation paths may break.

### Overhead profile (how to keep minimal)
- Tag allocation metadata with a compact enum (constant-time storage).
- Keep backend mapping mostly identical in v1; no broad allocator redesign.
- Add per-class counters via lock-minimal/per-CPU accounting.
- Use profile-gated validation (warn/telemetry first, strict enforcement later).

**Expected overhead target:** negligible allocator cost increase in v1, primarily bookkeeping.

## 2.3 Fault domains (`fault_domain_*`)

### How it helps
- Introduces explicit containment and restart boundaries beyond process-only semantics.
- Supports safety and reliability profiles by turning crashes into manageable domain-level events.
- Aligns naturally with service supervision and policy-driven recovery.

### Key risks
- **Policy leakage into kernel:** full supervision logic in ring-0 would increase complexity and attack surface.
- **Restart storms:** insufficient backoff or restart windows may destabilize system under repeated faults.
- **Ownership confusion:** unclear domain lifecycle (thread/process/service attachment rules) can create hard bugs.

### Overhead profile (how to keep minimal)
- Kernel performs tagging, accounting, and event emission only.
- Recovery orchestration remains in user-space supervisor services.
- Domain lookup should be O(1) handle-based; keep fault-path code branch-light.
- Start with thread attach in v1; broaden scope after lifecycle model hardens.

**Expected overhead target:** minimal steady-state cost; bounded extra work only on fault path.

## 3) Linux and Android personalities: use without performance penalty

## 3.1 Default behavior (no opt-in)
- Linux and Android personalities keep existing ABI/syscall behavior unchanged.
- No mandatory translation of legacy APIs to new primitives.
- No extra context-switch or syscall-layer emulation tax for unmodified apps.

## 3.2 Optional opt-in mapping (when useful)

### Linux personality
- `sched_setscheduler`, affinity, cgroup/uclamp-like policies can map to `intent_set`.
- DMA/network-heavy interfaces can request `MEM_CLASS_PACKET` / `MEM_CLASS_DMA`.
- Service groups can be mapped to fault domains for stronger recovery semantics.

### Android personality
- Task profiles / cpusets / foreground-background classes can map to intent hints.
- Media/AI memory paths can map to `MEM_CLASS_MODEL`/`MEM_CLASS_SENSOR_STREAM`.
- Critical system services can be grouped into explicit fault domains.

## 3.3 Why this avoids overhead
- Mapping is done at policy/personality edge only where explicitly configured.
- Legacy pathways remain direct.
- Kernel fast paths remain simple for workloads without attached metadata.

## 4) Compatibility and ABI strategy (must-have guardrails)

1. **All primitives additive in v1** (no replacement of legacy create/alloc interfaces).
2. **Versioned structs in UAPI** with reserved fields for forward extension.
3. **Standalone syscalls first** (`intent_set/get`, `fault_domain_create/...`, class-aware alloc path).
4. **Explicit degrade semantics** per profile for unsupported hints.
5. **No silent behavior break** for Linux/Android personalities.

## 5) Recommended v1 rollout order

1. **Memory classes first** (lowest ABI risk, immediate observability value).
2. **Fault domains second** (high reliability payoff, clean service-supervisor integration).
3. **Intent set/get third** (high strategic value, but easiest to overdesign).

This ordering yields tangible progress while containing complexity and regression risk.

## 6) Risk register and mitigations

| Risk | Probability | Impact | Mitigation |
|---|---:|---:|---|
| Policy divergence across profiles | Medium | High | Define profile-specific conformance tables and tests for each semantic field. |
| Scheduler overhead from intent evaluation | Medium | Medium | Evaluate hints at enqueue/wakeup, cache decisions, keep tick path lean. |
| Fault-domain restart storms | Medium | High | Require restart windows/backoff in attributes and supervisor policy. |
| Allocation-class misuse by callers | High | Medium | Add lint/static checks + runtime telemetry + staged enforcement. |
| Personality compatibility regression | Low | Critical | Keep primitives fully additive and avoid mutating legacy syscall ABI. |

## 7) Performance acceptance gates (before broad enablement)

- **Syscall latency:** new syscalls within defined micro-bench budget relative to baseline object syscalls.
- **Scheduler overhead:** no significant regression on legacy workloads; bounded overhead on intent-enabled workloads.
- **Allocator throughput:** class tagging path within acceptable delta versus baseline alloc/free.
- **Fault path:** domain tagging/event emission bounded; no lock contention amplification under fault bursts.
- **Personality benchmarks:** Linux and Android compatibility suites show no meaningful regression when mappings are disabled.

## 8) Practical adoption model for your “personality-first” goal

- Keep personalities **strictly compatibility-first by default**.
- Offer **feature flags** to enable mapping from Linux/Android constructs to native primitives per subsystem.
- Roll out mapping by vertical slice (scheduler first, then memory, then fault supervision), not all at once.
- Use telemetry from real workloads before turning any mapping to default-on.

## 9) Final recommendation

Yes, these primitives can work **without much overhead or performance penalty** if Bharat-OS keeps them additive, metadata-first in v1, and policy-driven in services.

For immediate implementation, focus on:
- `mem_class_t` tagging/accounting,
- `fault_domain_t` object + fault tagging/event path,
- `intent_set/get` attachment/query (not replacement create syscall).

That path preserves Linux/Android ABI stability while opening a clean route to modern heterogeneous runtime behavior.
