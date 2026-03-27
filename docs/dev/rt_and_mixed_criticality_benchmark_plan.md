---
title: RT and Mixed-Criticality Benchmark Plan
status: Draft
version: 1.0
owner: Core QA / Architecture
reviewers: Core Maintainers
last_updated: 2024-03-24
tags:
  - benchmarking
  - verification
  - realtime
  - mixed-criticality
---

# Bharat-OS RT and Mixed-Criticality Benchmark Plan

This document outlines the rigorous engineering benchmarks required to validate the mixed-criticality architecture of Bharat-OS. The goal is to prove, through data, that the capability kernel provides the necessary deterministic mechanisms and that the service layer successfully enforces profile contracts.

## 1. Goals

*   Validate that RT workloads meet their deadlines predictably.
*   Prove that GP workloads cannot compromise RT execution (interference testing).
*   Quantify primitive costs (context switches, latencies, allocators).
*   Measure failure containment efficiency.
*   Establish reproducible baselines for continuous integration.

## 2. Terminology

*   **Jitter:** The deviation from the expected timing of an event (e.g., periodic wakeups).
*   **Deadline Miss:** When a task fails to complete its critical section within its defined timeframe.
*   **Interference:** The impact of non-critical (GP) tasks on the timing or resources of critical (RT) tasks.
*   **Contamination:** A fault or state corruption crossing a fault domain boundary.

## 3. Hardware / Environment Targets

All benchmarks must be executable and reproducible in at least two environments:
1.  **QEMU (e.g., x86_64, aarch64):** For CI/CD automation and regression testing.
2.  **Physical Board (Target specific, e.g., an NXP, STM32, or RPi4):** For true hardware-level latency validation.

## 4. Benchmark Suite Structure

Do not rely on an “overall benchmark score”. The focus is on specific **contract benchmarks.**

### 4.1 Microbenchmarks
*Objective: Prove primitive costs.*

Measure the following, reporting min, median, p99, p99.9, max, and stddev:
*   Interrupt latency
*   Timer expiry latency
*   Scheduler dispatch latency
*   Context switch latency
*   IPC one-way latency
*   IPC round-trip latency
*   Cross-core handoff latency
*   Page-fault handling latency
*   Memory allocation latency by class (`MEM_RT` vs `MEM_NORMAL`)
*   Lock hold time / queue service time
*   Wakeup latency from IRQ to runnable thread

### 4.2 Interference Benchmarks
*Objective: Prove isolation under stress.*

Run a clean RT workload while injecting the following GP noise:
*   Network flood
*   Storage writes
*   Memory pressure (reclaim churn)
*   Telemetry flood
*   Mixed interrupt storm
*   Cross-core message traffic

*Metric:* Jitter of the RT workload must stay within defined, bounded limits.

### 4.3 Partition/Isolation Benchmarks
*Objective: Prove structural contracts.*

Measure:
*   Can GP traffic delay an RT control channel? (Expected: No)
*   Can GP memory reclaim impact an RT allocation class? (Expected: No)
*   Can non-critical faults remain inside their fault domain? (Expected: Yes)
*   Can telemetry remain enabled without violating the RT timing envelope? (Expected: Yes)

### 4.4 Recovery Benchmarks
*Objective: Prove fault resilience.*

Measure:
*   Restart time of a non-critical service
*   Effect of non-critical crash on RT loop (Expected: Zero effect)
*   Degraded-mode transition time
*   Watchdog reaction time
*   Safe-state transition latency

### 4.5 Profile Benchmarks
*Objective: Prove policy differences.*

Run the same workload under RT, GP, and MIX profiles to demonstrate differing behaviors and guarantees (e.g., GP drops packets, RT guarantees delivery; GP yields CPU, RT preempts).

## 5. Minimal Benchmark Matrix (Phase 1)

These are the immediate targets to implement.

### BM-1: Timer Determinism
*   **Setup:** 1 kHz and 10 kHz periodic task.
*   **Metrics:** Wakeup jitter, missed deadlines, overrun count.

### BM-2: Scheduler Latency Under Load
*   **Setup:** One high-priority RT task, several medium GP tasks, background memory churn.
*   **Metrics:** Dispatch delay, deadline miss count, worst-case jitter.

### BM-3: IPC Critical-Channel Benchmark
*   **Setup:** RT producer to RT consumer, then inject GP IPC traffic.
*   **Metrics:** One-way latency, queue backlog growth, head-of-line blocking.

### BM-4: IRQ-to-Thread Latency
*   **Setup:** Hardware/QEMU interrupt triggers critical thread activation.
*   **Metrics:** IRQ arrival to handler, handler to runnable, runnable to execution start.

### BM-5: Fault Containment Benchmark
*   **Setup:** Crash a GP service while an RT control loop runs.
*   **Metrics:** RT jitter before/during/after fault, restart latency, contamination outside fault domain.

### BM-6: Memory Class Benchmark
*   **Setup:** Compare `MEM_RT`, `MEM_NORMAL`, `MEM_DMA` behavior under allocation pressure.
*   **Metrics:** Allocation latency, failure rate under stress, reclaim side effects.

### BM-7: Cross-Core Handoff Benchmark
*   **Setup:** Critical thread pinned on one core, GP load on others.
*   **Metrics:** Remote wakeup delay, handoff completion time, TLB shootdown side effect.

## 6. Required Code Instrumentation

To support these benchmarks, the kernel and services must be instrumented with low-overhead tracepoints for:
*   Scheduler enqueue/dequeue
*   Dispatch start/end
*   Timer arm/fire
*   IRQ enter/exit
*   IPC send/receive/drop
*   Queue depth high-water mark
*   Deadline miss
*   Watchdog trip
*   Fault-domain enter/recover
*   Alloc/free latency by memory class

*(These must adhere to the common telemetry contract defined in Bharat-OS).*

## 7. Definition of Success

For the first serious mixed-criticality milestone, success is defined as:
1.  RT periodic task meets deadline under idle load.
2.  RT periodic task still meets deadline under GP CPU stress.
3.  RT control channel latency stays within bound under mixed traffic.
4.  GP service crash does not break RT control loop.
5.  MIX profile shows bounded degradation, not collapse.
6.  All results are reproducible across at least QEMU and one physical board.
