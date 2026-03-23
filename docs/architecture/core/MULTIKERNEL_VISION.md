---
title: Multikernel (Per-Core) Architecture Vision & Maturity
status: Draft
owner: Architecture Team
version: 1.0.0
last_updated: 2024-05-15
tags: [multikernel, barrelfish, real-time, automotive, architecture]
---

# Multikernel Vision and Subsystem Maturity

Bharat-OS is transitioning towards a highly decoupled, Barrelfish-like **multikernel architecture**, where each CPU core operates with high autonomy and communication between cores happens via strict message passing (uRPC/IPC) rather than implicit shared memory. This is foundational for our ability to cleanly support diverse profiles (Real-Time, General Purpose, Hybrid) across heterogeneous SoC domains (e.g., Automotive, Edge, Robotics, Drones) spanning x86_64, arm64, arm32, riscv64, and riscv32 hardware.

The goal is to eliminate global locks, unbounded contention, and cross-core cache invalidation storms in fast paths, guaranteeing 99% of the multikernel properties necessary to meet stringent Real-Time (RT) predictability while retaining GP flexibility where appropriate.

## Core Architectural Paradigm
- **Per-Core State**: Schedulers, capability tables, and runqueues are already heavily partitioned per-core (`g_cpu_locals[core_id]`).
- **Message Passing (uRPC)**: Cross-core operations (e.g., TLB shootdowns, process migrations, capability revocations) must happen via explicit `uRPC` messages, completely avoiding cross-core shared spinlocks.
- **Fail-Fast & Bounded Execution**: Fast paths must never perform unbounded loops over shared state.

## Subsystem Maturity Analysis

Below is an assessment of the current state of core kernel subsystems against our multikernel vision, highlighting what needs to mature.

### 1. Scheduler (`kernel/src/sched.c`)
- **Current State**: High maturity for multikernel. Uses purely per-core runqueues (`g_cpu_locals[core].runqueue`), per-core locks, and explicit migration via core affinity masks.
- **Needs to Mature**:
  - **Cross-core thread operations**: Global thread array `g_threads` and `g_processes` are shared. While slots are allocated atomically or locked locally, finding a thread by ID still scans a global list.
  - **Thread Reaper**: `sched_reap_terminated_threads` uses a global lock (`g_reap_lock`), which could become a contention point across cores. Reaping should ideally be localized per-core or delegated to a specific GP core via message passing.

### 2. Physical Memory Manager (PMM) (`kernel/src/mm/pmm/pmm.c`)
- **Current State**: Medium maturity. PMM is NUMA-aware, which aligns well with multikernel concepts, but it currently relies on NUMA node-level locks (`zone_t.lock`).
- **Needs to Mature**:
  - **Per-Core Caches**: To truly realize the multikernel vision, PMM needs per-core page magazines (caches) to satisfy >95% of allocations completely lock-free. Falling back to the NUMA node lock should be a slow-path exception.
  - **Page Reference Counting**: `pmm_ref_put` relies heavily on atomic compare-and-swap (CAS) operations on shared page metadata. In a strict message-passing system, releasing remote pages might benefit from deferred bulk-free messages to the owning core.

### 3. Translation Lookaside Buffer (TLB) (`kernel/src/mm/tlb/tlb_coordinator.c`)
- **Current State**: Medium maturity. The system correctly uses URPC and mailbox messaging to trigger remote TLB shootdowns rather than stalling cores.
- **Needs to Mature**:
  - **Shootdown Coordination**: The `g_pending_requests` array used to track TLB invalidation acknowledgments is protected by a global spinlock (`g_pending_requests_lock`). This creates a cross-core serialization bottleneck during heavy memory unmap operations.
  - **Path Forward**: A lock-free ring buffer or purely per-core tracking structures for pending URPC requests should replace the global lock.

## 3.1 Memory Protection Architecture
The page-table and memory protection model is fundamental to the multikernel approach. Rather than relying on a shared, global page table structure with cross-core locking, Bharat-OS employs per-core local page-table roots and uses strict uRPC shootdowns for remote TLB invalidation. The architecture cleanly separates the CPU→memory protection axis (MMU/MPU) from the Device→memory axis (IOMMU) into a unified Memory Protection Architecture (MPA) abstraction.

For complete details on the implementation format, HAL vtable, and frame ownership invariant, refer to the [Multikernel Memory Protection Architecture](multikernel-memory-protection-architecture.md) specification.

### 4. Interrupts (IRQ) & IPI (`hal/interrupt_common.c`)
- **Current State**: High maturity. IRQ routing natively supports affinity, and deferred work (Bottom-Halves) is implemented via strictly per-core queues (`g_deferred_queues[cpu_id]`).
- **Needs to Mature**:
  - **Global IRQ Descriptor Lock**: The `g_irq_descriptors` array uses a spinlock per IRQ line. While partitioned by IRQ, configuring shared IRQs across cores could still cause minor contention. However, since IRQ configuration is usually a slow-path/boot-time operation, this violation is highly tolerable for the RT vision.

### 5. Capabilities & Security (`kernel/src/capability.c`)
- **Current State**: Medium maturity. The model correctly uses per-core capability tables (`g_cpu_locals[i].cap_table`), which is excellent for multikernel design.
- **Needs to Mature**:
  - **Delegation/Revocation Locks**: `cap_table_delegate` has been partially refactored to use uRPC for cross-core capability delegation instead of cross-table spinlocks. It routes remote destination-table mutation through uRPC while retaining a synchronous API semantics via polling for an ACK. `cap_table_revoke` uses a similar synchronous polling approach for ACKs. In a strict Barrelfish model, altering a remote core's capability table must be completely asynchronous. The next evolution is full asynchronous APIs for both delegation and revocation.
  - **Tree Traversal**: Revocation traverses sibling lists that might span across tables owned by different cores. This logic must be refactored to use distributed state protocols (e.g., 2-phase commit or asynchronous distributed revocation).

### 6. Inter-Process Communication (IPC / uRPC) (`kernel/src/urpc/urpc_channel.c`)
- **Current State**: Medium maturity. The bootstrap mechanism exists to send BIND/ACK messages between cores.
- **Needs to Mature**:
  - **Performance**: The current uRPC implementation relies heavily on basic array state tracking (`g_urpc_states`). To support high-throughput GP and low-latency RT profiles, uRPC must mature into a fully cache-aligned, lock-free ring buffer architecture (e.g., using shared memory purely for unidirectional queues with memory barriers).

## Acceptable Bottlenecks (Tolerable Violations)
To meet our 99% threshold without over-engineering, we accept the following shared state:
- **Global Thread/Process ID Counters**: Using atomic increments for `g_next_thread_id` is acceptable as thread creation is a slow-path operation.
- **Hardware Topology Discovery**: Boot-time discovery of memory regions, CPUs, and IRQ routing tables can remain globally shared and read-only post-boot.
- **NUMA Zone Locks**: While per-core PMM caches are needed, retaining spinlocks on the fallback NUMA-level PMM zones is acceptable for memory pressure scenarios.

## Next Steps
1. Refactor TLB Coordinator to remove `g_pending_requests_lock`.
2. Implement per-core page caching in PMM to eliminate fast-path zone locking.
3. **[Partially Implemented]** Transition Capability cross-core delegation to use strictly uRPC messages instead of cross-core spinlocks (delegation now routes cross-core destination mutation through uRPC while retaining synchronous API semantics). Full asynchronous APIs are the next evolution.
