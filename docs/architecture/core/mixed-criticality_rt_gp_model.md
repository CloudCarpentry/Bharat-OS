---
title: Mixed-Criticality RT & GP Model
status: Draft
version: 1.0
owner: Architecture Team
reviewers: Core Maintainers
last_updated: 2024-03-24
tags:
  - architecture
  - realtime
  - mixed-criticality
  - scheduling
  - profiles
---

# Mixed-Criticality Architecture in Bharat-OS

Bharat-OS does not utilize a rigid “one OS per device type” or a binary "RT kernel vs. GP kernel" split. Instead, the architectural foundation of Bharat-OS is built upon a **small capability kernel + profile-driven policy/services + mixed-criticality partitions/channels**.

This approach defines deterministic mechanisms within the kernel and delegates complex policy decisions to user-space services, enabling multiple profiles to coexist safely within the same system.

## 1. Defining Criticality in Bharat-OS

In Bharat-OS, **criticality is a contract, not a marketing label.**

Criticality defines the guarantees a workload receives regarding time, memory, execution, and isolation. It is enforced through strict separation of mechanisms (in the kernel) and policies (in services).

Bharat-OS defines three core workload profiles:

*   **RT (Real-Time) Profile**: Designed for control loops, hard deadlines, and safety-critical execution. Characterized by deterministic scheduling, bounded IPC, pinned memory classes, and tight interrupt routing.
*   **GP (General Purpose) Profile**: Designed for throughput, fairness, and complex system services. Characterized by best-effort scheduling, dynamic memory with background reclaim, and richer (but potentially unbound) service interaction.
*   **MIX (Mixed) Profile**: Represents the coexistence rules and isolation contracts that allow RT and GP workloads to run concurrently on the same hardware, defining explicit channel classes between critical and non-critical domains.

## 2. Kernel vs. Service Responsibilities

To achieve mixed-criticality without bloating the kernel, responsibilities are strictly divided:

### Guaranteed by the Kernel (Mechanisms)
*   **Deterministic Scheduling**: Providing bounded context switch, interrupt delivery, and dispatch latencies.
*   **Memory Isolation**: Enforcing capability-based memory protection and dedicated memory classes (e.g., pinned memory for RT tasks).
*   **Bounded IPC/uRPC**: Providing non-blocking or strictly bounded messaging primitives and explicit channel classes.
*   **Fault Containment**: Enforcing fault domains to prevent a fault in one partition from affecting another.
*   **Time Primitives**: Providing precise timers and deadline tracking mechanisms.

### Delegated to Services (Policies)
*   **Routing & Admission**: `sysmgr` and `policymgr` dictate which workloads are admitted, their profiles, and how they interact.
*   **Degradation Rules**: Determining the system's response to overloads or failures (e.g., prioritizing RT tasks and gracefully degrading GP tasks).
*   **Telemetry & Health**: User-space services (`telemetryd`, `healthd`) monitor system health, collect traces, and detect contract violations.
*   **Complex Subsystems**: Filesystems, network stacks, and device governance.

## 3. Mixed-Criticality Contracts

The interactions between different profiles are strictly governed by explicit contracts:

*   **Channel Types**: Communication between critical (RT) and non-critical (GP) domains must use explicit channel classes that guarantee bounded latency and prevent priority inversion or queue exhaustion. GP traffic must never be able to delay an RT control channel.
*   **Admission Rules**: RT workloads must declare their memory, CPU, and IPC bounds upfront. The policy manager admits them only if the system can mathematically guarantee their deadlines.
*   **Failure Containment & Degradation**: Non-critical faults must remain inside their fault domain. The restart time of a non-critical service must have zero effect on an RT loop. If the system is overloaded, GP profiles will experience degraded performance or termination to protect RT profiles.

## 4. Policy Enforcement Matrix

The following table summarizes the policies enforced for different criticality profiles:

| Subsystem | RT Profile | GP Profile | MIX / Cross-Domain Contract |
| :--- | :--- | :--- | :--- |
| **Scheduling** | Deterministic, hard deadlines, preemption. | Throughput-focused, fair share, best effort. | RT tasks always preempt GP tasks. GP tasks cannot block RT execution. |
| **Memory** | Pinned (`MEM_RT`), pre-allocated, no background reclaim. | Dynamic (`MEM_NORMAL`), page faults allowed, background reclaim. | GP memory reclaim/faults cannot impact RT allocation or execution. |
| **IRQ Routing** | Tight routing, minimal handler latency, high priority. | Standard routing, handled via deferred work queues. | RT interrupts preempt GP interrupts and execution. |
| **IPC** | Bounded, non-blocking or bounded-wait, priority inheritance. | Standard queuing, potentially unbounded waits. | GP traffic cannot delay RT control channels. Explicit channel classes required. |
| **Faults** | Strict containment. Faults trigger immediate fail-safe or rapid restart. | Allowed to fail and restart without affecting the rest of the system. | GP fault must remain inside its fault domain; zero effect on RT loops. |
| **Telemetry** | Must remain enabled without violating the RT timing envelope. | Rich telemetry, allowed to consume background resources. | Telemetry traffic cannot flood or delay RT operations. |
| **Services** | Minimal, trusted critical services only. | All services available (network, storage, UI). | Cross-domain service calls must use strict boundary contracts. |
| **Isolation** | Hard isolation via strict capability boundaries. | Standard capability-based isolation. | GP cannot access RT memory or interrupt RT control flow. |
