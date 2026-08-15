---
title: ADR-032 - HAL feature and memops publication
status: Accepted
owner: Architecture Working Group
last_updated: 2026-08-13
tags: [architecture, hal, cpu, memory]
---

# ADR-032: Invert CPU discovery and freeze per-core memops selection

## Decision

Architecture code probes ISA-specific CPU state and publishes one normalized,
by-value `hal_cpu_feature_set_t` per online CPU during serial boot. HAL common
owns those records, computes the system intersection and union, and freezes
them before system queries succeed. HAL never interprets architecture-private
capability records.

Memops follows the same lifecycle. HAL common owns dispatch and the byte-only
scalar fallback. Architecture code may register a GPR-only backend per CPU
before freeze. Missing registration, pre-freeze dispatch, an unknown CPU,
early boot, or IRQ-safe execution selects the scalar fallback. There is no
unfreeze operation; frozen records and function tables are immutable and read
lock-free.

x86 registers REP only when CPUID reports ERMS usable on that CPU. Arm64,
Arm32, RV64, and RV32 register integer-only implementations. Every backend
provides copy, move, set, and compare; incomplete tables are rejected. Backend
context flags are checked by HAL, while implementation flags are descriptive
and contain no ISA-specific vocabulary. SIMD/vector state is outside this
contract.

BharatLibC does not call HAL. Its independent resolver starts on a byte-safe
fallback, accepts the versioned `bharat_cpu_features_v1_t` system-intersection
descriptor once, validates its size/version, and then publishes a lib-local
ISA implementation. A scheduler may expose a larger descriptor only when it
also constrains execution to CPUs that guarantee every exposed feature.

## Dependency and failure boundary

`hal_common` builds against HAL contracts and public interface/base types only.
`core/hal/common` must not import `arch/*` or `kernel/src/*`; libraries must not
import HAL internals or kernel-private sources. CI enforces this direction.
Invalid, incomplete, or late publication is rejected, feature queries fail closed until
freeze, and unknown DMA coherency is treated as non-coherent. No userspace
authority is conveyed by the CPU feature descriptor: the word "feature" is
deliberately used instead of the security meaning of capability.

## Consequences

Per-core selection supports heterogeneous CPUs without weakening the safe-on-
all-CPUs feature view. Cache, TLB, DMA, and entropy mechanisms should adopt the
same semantic-contract and architecture-registration pattern.
