---
title: Personality Performance Contract
status: active
owner: Architecture Team
version: 1.0
---

# Personality Performance Contract

### Contract Status
- **Spec**: ✅ Documented and versioned
- **Implemented**: 🚧 Pending core/kernel/service behavior merge
- **Validated**: ❌ Pending stress/fault-injection tests


Compatibility is not enough. Personality support on Bharat-OS must be performance-grade. If the Linux or Android personality acts as a slow translation layer, it will only be tolerated for demos. Our goal is **near-native semantics, near-native performance, and native-grade observability** for each personality.

## 1. The Core Rule

Do **not** build personalities as heavy emulation first. Instead, build them as:
*   Thin ABI/runtime personalities.
*   Running on top of high-performance shared kernel mechanisms.
*   Using zero-copy and direct fast paths where possible.
*   Enforcing policy separation only where it does not hurt hot paths.

## 2. 3 Execution Tiers

We enforce a tiered execution model to preserve performance parity.

### Tier 1 — Native Fast Path
For Bharat Native apps and services:
*   Direct ABI and IPC.
*   Direct graphics/network/storage contracts.
*   No compatibility tax.
*   **Must be the best-performing path.**

### Tier 2 — Personality-Optimized Path
For Linux and Android personalities:
*   ABI-compatible front end.
*   Shared kernel primitives underneath.
*   Compatibility shim *only* at the contract boundary.
*   No repeated translations across layers.

### Tier 3 — Fallback/Emulation Path
For unsupported or rare features:
*   Slow path via userspace helpers or complex translation.
*   Clearly marked and measurable.
*   **Must never dominate common workloads.**

## 3. Hot-Path First Design

For every personality, we separate operations into hot, warm, and cold paths. Hot paths must avoid extra service hops, dynamic allocation, format translation, and repeated validation.

### Hot Path Examples
*   **Linux:** `read`/`write`, `send`/`recv`, `epoll` wait/wakeup, futex-like primitives, `mmap`/page fault.
*   **Android:** Binder transact/reply, UI buffer queue, input event dispatch, audio callbacks, surface composition signaling.
*   **Native:** Service IPC, async runtime wakeups, graphics command submission, storage/network stream path.

## 4. Personality-Aware Optimizations

Performance parity requires tuning the shared kernel mechanisms to cater to personality semantics without rewriting them:

*   **Scheduler:** Per-personality hooks for wakeup latency, UI/render affinity, server throughput classes, and futex/park efficiency.
*   **IPC:** Different IPC classes: tiny control messages, large shared buffer handoff, and zero-copy async streams.
*   **VM:** COW performance, page fault fast paths, and personality-specific memory hints.

## 5. Performance Budgets

Every personality adapter operates under a strict budget compared to native benchmarks on identical hardware:

*   **Linux Personality Budget:** Tight overhead bounds on syscall adapter, VFS translation, socket translation, and thread/TLS overhead. Target is parity with native Linux for server/CLI workloads.
*   **Android Personality Budget:** Tight overhead bounds on Binder wrapper, graphics queue, and audio bridge overhead. Target is parity with native Android for UI/media workloads.

## 6. Observability Parity

To hit these budgets, the Bharat-OS SDK must provide tracing, perf counters, IPC latency histograms, frame timing, and page fault profiling equal to or better than native Linux/Android ecosystems.

## 7. Hard KPIs and Latency Targets

To ensure the "no translation tax" rule is upheld, the following hard KPIs are enforced for personality hot paths:

*   **Null Syscall Latency:** Must be within a tight delta (e.g., < 5%) of the equivalent native Linux `getpid` or `prctl` latency.
*   **Memory Mapping (`mmap`/`munmap`/`mprotect`):** Must match native speeds. The VM path must use a single authority without a double-layer translation.
*   **Futex Wait/Wake Latency:** Must bypass global locks and remain highly concurrent, matching Linux/Android native performance.
*   **Event Polling (`epoll`/`poll`/`select`):** Readiness mapping must not involve per-event translation churn; it must map directly to the kernel's readiness object model.
*   **Process / Thread Spawn Overhead:** Clone/fork-like operations must not be heavily penalized by personality setup.
*   **Shared-Memory IPC Throughput:** Must achieve zero-copy throughput parity for large buffers (critical for Binder and Ashmem-like behavior).

## 8. Benchmark Methodology

All performance validations must follow a strict comparison method:
*   **Environment:** Same hardware target (or accurate QEMU configuration for preliminary checks).
*   **Toolchain:** Same compiler class and optimization levels.
*   **Workload:** Identical benchmark source code (e.g., standard microbenchmarks for syscall, epoll, mmap).
*   **Baseline:** Native Linux result vs. Bharat Linux personality result.

Pass bands require that hot syscalls fall within the defined latency deltas, IPC fast paths perform zero extra memory copies, and no repeated personality/string lookups occur on internal kernel state transitions.
