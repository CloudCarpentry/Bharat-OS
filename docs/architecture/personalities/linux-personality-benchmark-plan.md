---
title: Linux Personality Benchmark Plan
status: active
owner: Architecture Team
version: 1.0
---

# Linux Personality Benchmark Plan

This document outlines the mandatory microbenchmarks and validation methodology for evaluating the performance of the Bharat-OS Linux personality. The goal is to enforce the "no translation tax" rule and prove that Linux compatibility hot paths run at near-native speeds.

## 1. Mandatory Benchmarks

The following operations define the latency profile of the compatibility layer. They must be continuously monitored against native baselines.

*   **Null Syscall Latency (`getpid`, `prctl`):** Measures the absolute overhead of the syscall trap, context switch, dispatch table lookup, and return path.
*   **Memory Mapping (`mmap`, `munmap`, `mprotect`):** Measures the efficiency of the single-authority VM layer handling standard Linux anonymous and file-backed mappings.
*   **Futex Wait/Wake Latency:** Measures the performance of user-space synchronization primitives. The wait/wake path must not hit global kernel locks and must scale across cores.
*   **Event Polling (`epoll` latency):** Measures readiness notification overhead. The test must validate that `epoll` maps directly to the kernel event system without per-event translation loops.
*   **Time Retrieval (`clock_gettime`):** Measures the VDSO fast-path vs. trap implementation for monotonic and realtime clock reads.
*   **Process/Thread Spawning (Thread Ping-Pong):** Measures `clone` overhead, TLS initialization, and cross-thread wakeups to validate the thread personality descriptor setup.

## 2. Comparison Methodology

To guarantee a fair comparison, all benchmarks must adhere to the following conditions:

*   **Environment:** Identical hardware (same CPU model, stepping, and cache layout). If running in QEMU/KVM for CI, the exact same machine configuration must be used.
*   **Toolchain:** Same compiler class (e.g., Clang 16+) and optimization flags (`-O3`, `-flto`) used to compile the benchmark payloads.
*   **Workload:** Identical benchmark source code (e.g., standard microbenchmarks or custom bare-metal equivalent tests) executed on both platforms.
*   **Baseline Definition:** The performance delta is calculated as: `(Bharat Linux Personality Result / Native Linux Result) - 1`.

## 3. Pass Criteria (Definition of Done)

A benchmark suite is considered successful only if it meets these strict criteria:

*   **Latency Overhead:** Hot-path syscall overhead must fall within a strict tight delta (e.g., < 5% variance) compared to native Linux.
*   **Zero-Copy IPC:** Shared-memory IPC, `mmap` handoffs, and event notifications must demonstrate zero extra memory copies within the kernel boundary.
*   **Architecture Validation:** The tracing output must prove there is no "double-fault" handling path for VM operations and no fallback emulation engaged during fast-path operations.
