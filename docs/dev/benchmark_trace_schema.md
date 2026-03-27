---
title: Benchmark Trace Schema
status: Draft
version: 1.0
owner: Core QA / Architecture
reviewers: Core Maintainers
last_updated: 2024-03-24
tags:
  - benchmarking
  - verification
  - telemetry
  - schema
---

# Benchmark Trace Schema

This document defines the standard JSON schema for all telemetry and benchmark traces collected by the Bharat-OS benchmark runner. This format must be adhered to by all automated tests designed to validate RT and mixed-criticality contracts.

The resulting JSON traces should be easily ingestible by external analysis tools (e.g., Python Pandas, Prometheus, or simple grep/awk scripts) to generate the min/median/p99/p99.9/max latencies required by the `rt_and_mixed_criticality_benchmark_plan.md`.

## 1. Top-Level Trace Format

Every trace file must be a JSON array containing a series of Event Objects.

```json
[
  { "event": "run_start", "timestamp": 1711284000000000, "profile": "RT", "benchmark": "BM-1", "target": "QEMU_AARCH64" },
  { "event": "timer_fire", "timestamp": 1711284000001000, "tid": 42, "expected_timestamp": 1711284000001000 },
  ...
  { "event": "run_end", "timestamp": 1711284010000000, "status": "PASS", "missed_deadlines": 0 }
]
```

## 2. Event Types and Required Fields

All events must contain the following baseline fields:
*   `event` (string): The type of the event.
*   `timestamp` (integer): The nanosecond precision monotonic timestamp.
*   `cpu` (integer): The logical core ID where the event occurred.
*   `tid` (integer): The thread ID context (0 if in interrupt context).

### 2.1 Scheduler Events

*   **`sched_enqueue`**: A thread became runnable.
    *   `priority` (integer): The scheduling priority.
    *   `queue_depth` (integer): The number of threads currently runnable on this CPU.
*   **`sched_dispatch`**: A thread was scheduled onto a CPU.
    *   `prev_tid` (integer): The thread that was preempted or yielded.
    *   `latency_ns` (integer): The time between `sched_enqueue` and `sched_dispatch`.

### 2.2 Interrupt Events

*   **`irq_enter`**: An interrupt handler began executing.
    *   `irq_num` (integer): The hardware IRQ number.
*   **`irq_exit`**: An interrupt handler completed.
    *   `duration_ns` (integer): The time spent in the handler.
    *   `woken_tid` (integer): The TID of any thread woken by this IRQ (or 0).

### 2.3 Timer / Deadline Events

*   **`timer_arm`**: A deadline timer was set.
    *   `target_timestamp` (integer): The requested absolute wakeup time.
*   **`timer_fire`**: A timer expired and a thread was woken.
    *   `expected_timestamp` (integer): The timestamp requested in `timer_arm`.
    *   `jitter_ns` (integer): The difference between `timestamp` and `expected_timestamp`.
*   **`deadline_miss`**: A thread failed to complete its work before its deadline.
    *   `deadline_timestamp` (integer): The hard deadline that was missed.
    *   `overrun_ns` (integer): By how much the deadline was missed.

### 2.4 IPC / uRPC Events

*   **`ipc_send`**: A message was queued to an endpoint.
    *   `endpoint_id` (integer): The destination capability/endpoint.
    *   `msg_size` (integer): Size of the payload.
*   **`ipc_receive`**: A message was dequeued.
    *   `sender_tid` (integer): The TID that originated the message.
    *   `latency_ns` (integer): Time spent in the queue.
*   **`ipc_drop`**: A message was dropped (e.g., due to strict cross-domain rate limits).
    *   `reason` (string): E.g., "QUEUE_FULL", "RATE_LIMIT", "DEADLINE_EXPIRED".

### 2.5 Fault Domain Events

*   **`fault_enter`**: A thread triggered a fault (e.g., page fault, invalid instruction).
    *   `fault_type` (string): Description of the hardware or software fault.
    *   `domain_id` (integer): The isolated fault domain encompassing this thread.
*   **`fault_recover`**: The `healthd` or `sysmgr` completed recovery.
    *   `action` (string): E.g., "RESTART", "DEGRADE", "KILL".
    *   `latency_ns` (integer): Time from `fault_enter` to service availability.

### 2.6 Memory Allocation Events

*   **`mem_alloc`**: A page or object was allocated.
    *   `class` (string): E.g., "MEM_RT", "MEM_NORMAL".
    *   `size` (integer): Bytes allocated.
    *   `latency_ns` (integer): Time spent inside the allocator (including any reclaim if applicable).
*   **`mem_free`**: A page or object was freed.
    *   `class` (string): E.g., "MEM_RT", "MEM_NORMAL".
    *   `latency_ns` (integer): Time spent freeing the object.

## 3. Tooling and Reproducibility

To ensure these traces are reproducible:
1.  **Zero-Allocation Path**: Telemetry tracepoints must execute without dynamic memory allocation. They must write to lock-free, pre-allocated ring buffers.
2.  **Telemetry Offload**: `telemetryd` (a GP service) must drain these buffers and format the JSON asynchronously, guaranteeing that the act of measuring RT jitter does not *cause* RT jitter.
