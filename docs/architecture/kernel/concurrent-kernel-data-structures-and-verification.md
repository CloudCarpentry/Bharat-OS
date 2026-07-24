---
title: Concurrent Kernel Data Structures and Verification
status: Draft
owner: Architecture Team
last_updated: 2026-05-18
tags:
  - architecture
  - kernel
  - concurrency
  - verification
---

# Concurrent Kernel Data Structures and Verification

## Executive Summary

As Bharat-OS scales to high core counts and utilizes ultra-fast storage, traditional locking mechanisms and rigid data structures become performance bottlenecks. This document outlines the strategy for introducing highly concurrent, RCU-safe, and verified data structures into the kernel. The goal is to move from coarse-grained locking to lock-free or wait-free designs where possible, while maintaining the core principles of per-core ownership and capability enforcement.

## Problem Statement

Current kernel implementations often rely on global registries and traditional trees (like Red-Black trees) protected by spinlocks or mutexes. As the number of cores increases, lock contention limits scalability. Furthermore, as storage hardware (NVMe) reaches millions of IOPS, the overhead of traditional filesystem metadata management becomes significant. Finally, as the kernel surface expands, there is a need for safe, verified extension points that do not compromise system stability.

## Scope and Non-Goals

### Scope
- Design of concurrent range-based indexes for VM management.
- Implementation of RCU-style synchronization primitives.
- Sparse object lookup structures (XArray/radix variants).
- Framework for verified kernel hooks and shared state.
- Evolution of storage metadata structures toward log-structured B-trees.
- Rust-based safe abstractions for core kernel primitives.

### Non-Goals
- Blindly copying Linux kernel implementations (e.g., Maple Tree, eBPF).
- Replacing all kernel data structures at once.
- Implementation of a full JIT for kernel hooks (initially focused on verification).

## Design Principles

- **Mechanism vs. Policy:** Kernel owns mechanism; services own policy.
- **Per-Core Ownership:** Data structures should prefer per-core locality to minimize cross-core synchronization.
- **Lock-Free by Default:** Favor RCU and lock-free designs for read-mostly paths.
- **Capability Enforced:** Every data structure access must be compatible with Bharat-OS's capability model.
- **Profile-Gated:** Scalable structures are for `MMU_FULL` / high-performance profiles; simple fallbacks must exist for `MPU` / `MMU_LITE`.
- **No Unbounded Wait:** Avoid designs that can lead to priority inversion or indefinite spinning.
- **No Hidden Global Mutable Authority:** All shared state must be explicit and managed.

## Bharat-OS Mapping Table

| Topic                        | Bharat-OS mapping                                              |
| ---------------------------- | -------------------------------------------------------------- |
| Maple Tree-style range index | VM regions / address-space lookup                              |
| RCU                          | Read-mostly kernel registries, device registry, routing tables |
| XArray/radix-style index     | Page cache, object ID lookup, capability table indexes         |
| eBPF-like verifier           | Safe kernel extension / policy hook / tracing programs         |
| eBPF maps                    | Verified shared state between kernel/services                  |
| Log-structured B-tree        | Storage metadata/indexing roadmap                              |
| Rust-safe abstractions       | Safe wrappers for lists, locks, refs, queues                   |

## Component Designs

### VM Range Index (Maple Tree-inspired)
**Goal:** Replace linear or RB-tree VM region lookups with an RCU-safe B-tree variant.
- **Design:** A tree optimized for range lookups (e.g., finding the VMA containing a specific virtual address). It supports overlapping ranges (though VM regions shouldn't overlap) and gaps.
- **Concurrency:** Uses RCU for readers. Writers use a per-address-space lock, but do not block readers.
- **Bharat-OS Alignment:** Integrates with the `vm_manager` service. Logical path: `kernel/src/ds/`, current repo: `core/kernel/src/ds/`.

### RCU-style read-mostly primitive
**Goal:** Provide a standard mechanism for lockless reads of shared data.
- **Design:** Provides `rcu_read_lock()`, `rcu_read_unlock()`, and `synchronize_rcu()` or `call_rcu()`.
- **Implementation:** Leverages the uRPC mechanism for grace-period detection across cores in the multikernel model.
- **Alignment:** Essential for registries like the device manager and capability tables. Logical path: `kernel/src/sync/`, current repo: `core/kernel/src/sync/`.

### XArray/radix-style object index
**Goal:** Efficient mapping of integer IDs (TIDs, PIDs, CapIDs) to object pointers.
- **Design:** A multi-level radix tree that supports sparse indexes and is memory-efficient for small and large sets.
- **Concurrency:** RCU-safe lookups. Internal nodes use bit-masking for fast traversal.
- **Alignment:** Replaces global arrays and linked lists in the scheduler and object manager. Logical path: `kernel/src/ds/`, current repo: `core/kernel/src/ds/`.

### eBPF-inspired verified hook framework
**Goal:** Safe execution of small, bounded programs within the kernel.
- **Design:** A bytecode-based engine with a strict verifier.
- **Verification:** Mandatory static analysis to prove termination (no unbounded loops), memory safety (no OOB), and restricted helper usage.
- **Alignment:** Used for security policy hooks, tracing, and packet filtering. Logical path: `kernel/src/verify/`, current repo: `core/kernel/src/verify/`.

### eBPF-map-inspired shared state objects
**Goal:** Structured, concurrent data sharing between verified hooks and services.
- **Types:** Hash maps, arrays, ring buffers (LPM tries for networking).
- **Concurrency:** Native support for atomic operations and lock-free updates.
- **Alignment:** Provides the "memory" for kernel hooks, managed via capabilities.

### Log-structured B-tree storage metadata strategy
**Goal:** High-throughput, wear-leveling-friendly metadata management for NVMe.
- **Design:** Writes metadata changes in a log-structured manner to a B-tree. Reduces random writes and write amplification.
- **Consistency:** Atomic updates via copy-on-write (COW) at the tree level.
- **Alignment:** Roadmap target for `stacks/storage/metadata/`.

### Memory-safe abstraction strategy (Rust)
**Goal:** Eliminate memory safety bugs in core primitives.
- **Design:** Use Rust's ownership model to wrap C structures (lists, spinlocks, refs).
- **Implementation:** Gradual introduction of Rust components in `core/kernel/src/rust/` (where supported).
- **Alignment:** Supports the "Correctness First" goal of Bharat-OS.

## Profile Gating

| Structure | Tiny/MPU Profile | Desktop/Server (MMU_FULL) |
| --------- | ---------------- | ------------------------- |
| VM Index  | Simple List      | Concurrent Maple Tree     |
| Registry  | Fixed Array      | XArray + RCU              |
| Hooks     | Static C-hooks   | Verified Bytecode Engine  |
| Storage   | Simple FAT/Flat  | Log-structured B-tree     |

## Verification and Test Strategy

- **Formal Verification:** Use model checking (e.g., TLA+) for new RCU and lock-free algorithms.
- **Unit Tests:** Comprehensive coverage of edge cases (sparse fills, tree rebalancing).
- **Stress Tests:** Run on high-core-count simulators (or hardware) with randomized interleaving to catch race conditions. Mandatory for SMP-sensitive paths.
- **KASAN/TSAN:** Utilize sanitizer builds to detect memory errors and data races during testing.

## Benchmark Strategy

- **Micro-benchmarks:** Measure cycles per lookup/update vs. core count.
- **Contention analysis:** Measure "lock-wait" time under heavy parallel load.
- **End-to-end:** Measure system-call latency and throughput (e.g., page fault rate, IPC message rate). Compare old path vs. new path performance.

## Rollout Plan

1. **Phase 1:** Introduce RCU primitive and benchmark VM range index replacement.
2. **Phase 2:** Implement XArray for capability and thread lookups.
3. **Phase 3:** Pilot verified hooks in the network stack.
4. **Phase 4:** Integrate log-structured B-tree into the storage stack.

## Open Questions

- How to handle RCU grace periods in extremely low-latency/real-time profiles?
- What is the optimal bytecode for kernel hooks (eBPF vs. custom minimalist)?
- How to balance memory overhead of concurrent structures on memory-constrained (Tiny) boards?
