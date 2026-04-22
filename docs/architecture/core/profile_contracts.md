---
title: Profile Contracts and Mechanisms
status: Draft
version: 1.0
owner: Architecture Team
reviewers: Core Maintainers
last_updated: 2024-03-24
tags:
  - architecture
  - contracts
  - mechanisms
  - mixed-criticality
---

# Bharat-OS Profile Contracts and Mechanisms

### Contract Status
- **Spec**: ✅ Documented and versioned
- **Implemented**: 🚧 Pending kernel/service behavior merge
- **Validated**: ❌ Pending stress/fault-injection tests


The capability kernel of Bharat-OS avoids hardcoding policy or profile data natively. Instead, it relies on a standardized set of declarative primitives and mechanism contracts to enforce the mixed-criticality execution model. The architecture mandates that these first-class concepts are not scattered behaviors but strict contracts binding user-space policy to kernel capabilities.

This document outlines the core contracts required to implement the RT, GP, and MIX profiles in Bharat-OS.

## 1. Profile Descriptors

A Profile Descriptor (`profile.h`) is the primary mechanism by which the `sysmgr` informs the kernel and other services of the expectations of a workload.

**Contract Guarantees:**
*   A descriptor must explicitly define memory classes (`MEM_RT`, `MEM_NORMAL`), deadline constraints, and allowed IPC channel classes.
*   Admission rules rely entirely on these descriptors; the kernel or `sysmgr` will reject a workload if the profile descriptor demands resources the system cannot guarantee (e.g., an RT workload requesting unpinned memory or blocking IPC).
*   Profile descriptors are immutable once a fault domain or execution environment is instantiated.

## 2. Subsystem Capability and Registration Filter

Subsystems (e.g., the network stack, the storage stack) do not implicitly grant access to all workloads. A subsystem registration filter enforces capability-based access control.

**Contract Guarantees:**
*   A subsystem only accepts connections or routes requests from workloads that possess the corresponding capability, as granted by the `sysmgr` and defined in the profile descriptor.
*   GP workloads attempting to bind to an RT-critical subsystem (e.g., a critical CAN bus interface) will be explicitly rejected by the capability mechanism before any routing or policy logic is executed.

## 3. Deadline and Timer Classes

Timing is a fundamental primitive in Bharat-OS. The kernel must track deadlines, not just raw execution priorities.

**Contract Guarantees:**
*   Timers and deadlines (`deadline.h`) are classified by strictness: soft deadlines (GP) vs. hard deadlines (RT).
*   The scheduler guarantees that RT tasks with hard deadlines preempt any GP tasks or soft-deadline RT tasks.
*   A deadline miss is considered a contract violation. For RT profiles, this triggers an immediate fault or fail-safe state transition.

## 4. Fault Domains and Restart Policy

Fault Domains (`fault_domain.h`) define the isolation boundaries for failure containment and recovery.

**Contract Guarantees:**
*   A fault domain encapsulates all threads, capabilities, and memory resources of a service or workload.
*   A crash, exception, or resource exhaustion within a GP fault domain must have zero side effects on an RT fault domain.
*   The restart policy is explicitly defined: GP domains can be restarted dynamically by the `healthd`, while RT domain failures typically require a transition to a safe state or a degraded operational mode.

## 5. Per-Core uRPC/IPC Endpoint Classes

Communication channels between workloads must be typed to prevent priority inversion, queue exhaustion, or arbitrary delays.

**Contract Guarantees:**
*   Channel classes (`channel_classes.h`) dictate the transport semantics: RT-to-RT (bounded latency, priority inheritance), GP-to-GP (best effort, blocking allowed), and Cross-Domain (strict rate-limiting, non-blocking on the RT side).
*   A GP workload cannot delay an RT workload by flooding a cross-domain channel. The kernel enforces bounds on queue depth and message rate based on the endpoint class.

## 6. Memory Allocation Classes

Memory must be allocated according to the strict requirements of the profile.

**Contract Guarantees:**
*   `MEM_RT`: Pinned, contiguous (if required), pre-allocated physical memory. Never subject to background reclaim, swapping, or page faults during execution.
*   `MEM_NORMAL`: Standard dynamic memory, subject to demand paging and background reclaim.
*   A GP workload triggering heavy memory reclaim must not impact the allocation latency or execution speed of an RT workload using `MEM_RT`.

## 7. Telemetry Counters for Timing and Isolation

Telemetry is not an afterthought; it is a required contract for validating the mixed-criticality architecture.

**Contract Guarantees:**
*   The kernel provides low-overhead tracepoints (e.g., scheduler enqueue/dequeue, IRQ latency, IPC drops).
*   Telemetry collection (`telemetryd`) must operate under a GP profile, guaranteeing it does not violate the timing envelope of RT workloads.
*   Trace schemas (`benchmark_trace_schema.md`) are standardized to allow offline analysis of jitter, latency, and isolation boundaries.
