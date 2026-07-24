---
title: Bharat-OS Kernel Data Structures and Algorithmic Foundations
status: Active
owner: Architecture Team
last_updated: 2026-05-20
tags:
  - architecture
  - kernel
  - data-structures
  - algorithms
  - roadmap
---

# Bharat-OS Kernel Data Structures and Algorithmic Foundations

## 1. Purpose

Introduce production-grade kernel algorithms and verification foundations required for scalable VM, object indexing, read-mostly metadata, safe hooks, storage metadata, and memory-safe kernel coding. This document audits kernel-owned data structures and defines the canonical DS layer for Bharat-OS.

## 2. Core Principle: Algorithm Placement Follows Ownership

Algorithm is not selected because it is advanced. Algorithm is selected because it matches the ownership, latency, memory, and security boundary of that subsystem.

Bharat-OS must not place algorithms only by convenience. Every algorithm must be placed according to ownership, latency criticality, architecture dependency, and policy weight.

- **Mechanism** belongs in the kernel, HAL, or drivers.
- **Policy** belongs in services, stacks, or personalities.

## 3. Canonical Repository Paths

The canonical locations for kernel algorithmic foundations and data structures are:

| Component | Repository Path |
|---|---|
| Core Kernel Sources | `core/kernel/src/` |
| Kernel DS Headers | `core/kernel/include/bharat/kernel/ds/` |
| Kernel DS Sources | `core/kernel/src/ds/` |
| Scheduler | `core/kernel/src/sched/` |
| Memory Management | `core/kernel/src/mm/` |
| Capabilities | `core/kernel/src/cap/` |
| IPC / uRPC | `core/kernel/src/ipc/`, `core/kernel/src/urpc/` |
| System Services | `core/services/system/` |

## 4. Algorithm Placement Matrix

| Algorithm Type | Correct Location | Reason |
|---|---|---|
| Per-core run queues, priority queues, deadline queues | `core/kernel/src/sched/` | Scheduler mechanism; latency-critical |
| TLB shootdown queue/ring, ack bitmap, timeout tracking | `core/kernel/src/mm/` + HAL contract | Cross-core memory correctness |
| Page allocator magazines, free lists, bitmap allocators | `core/kernel/src/mm/pmm/` | Core memory mechanism |
| VM region lookup tree | `core/kernel/src/mm/vm/` | Address-space mechanism |
| Capability lookup table, generation counters, revoke list | `core/kernel/src/cap/` | Security-critical authority path |
| IPC/URPC ring buffer, message queue, endpoint registry | `core/kernel/src/ipc/`, `core/kernel/src/urpc/` | Cross-core/service mechanism |
| Driver/device matching tables | `core/drivers/registry/`, `core/drivers/core/` | Driver-owned discovery/matching |
| Network route table, ARP/neighbor cache, packet queues | `core/services/network/` or `core/stacks/net/` | Networking stack/policy, not core kernel |
| Filesystem path tree, mount namespace, fd table policy | `core/services/system/filesystem/` | VFS policy belongs outside kernel |
| Generic reusable structures | `core/kernel/src/ds/` | Shared building blocks |

## 5. Naming Convention

### Global Rule
Use `bh_` for internal Bharat-OS primitives.
Use `bharat_` only for stable UAPI/ABI-facing names where readability and external identity matter.

### Type Naming
| Kind | Format | Example |
|---|---|---|
| Struct typedef | `bh_<noun>_t` | `bh_ring_t` |
| Enum typedef | `bh_<domain>_<noun>_t` | `bh_sched_class_t` |
| Enum value | `BH_<DOMAIN>_<VALUE>` | `BH_MEM_CLASS_DMA` |
| Function | `bh_<module>_<verb>` | `bh_ring_push` |
| Static helper | `<module>_<verb>_local` | `ring_push_local` |
| Constants | `BH_<MODULE>_<NAME>` | `BH_RING_MIN_CAPACITY` |

## 6. Hot-Path and Small-Device Rules

### Hot-Path Rules
- Avoid heap allocation.
- Prefer intrusive structures.
- Prefer per-core ownership.
- Avoid global locks.
- Name lock-aware APIs clearly: `_locked`, `_nolock`, `_try`, `_irqsave`.

### Small-Device, MMU-Lite, and MPU-Only Constraints
- Provide fixed-capacity or compile-time bounded mode.
- Avoid unbounded dynamic growth.
- If a primitive requires an MMU, document the restriction.

## 7. Canonical Kernel DS Layer

### 1. Intrusive Doubly Linked List (`bh_list.h`)
Use for: scheduler runnable queues, wait queues, timer buckets, object lifecycle lists.
- `bh_list_node_t`
- `bh_list_init`
- `bh_list_push_back`
- `bh_list_remove`

### 2. Bitmap Allocator (`bh_bitmap.h`)
Use for: small ID allocation, CPU masks, page-frame availability map, capability slot availability.
- `bh_bitmap_t`
- `bh_bitmap_find_first_clear`
- `bh_bitmap_set_range`

### 3. Ring Buffer (`bh_ring.h`)
Use for: IPC/URPC queues, TLB shootdown requests, telemetry event buffers.
- `bh_ring_t`
- `bh_ring_push`
- `bh_ring_pop`

### 4. Generation-Based Handle Table (`bh_handle_table.h`)
Use for: capability handles, process/thread handles, IPC endpoint handles.
Purpose: prevent stale handle reuse, support revoke and lifecycle validation.
- `bh_handle_table_t`
- `bh_handle_alloc`
- `bh_handle_lookup`
- `bh_handle_revoke`

### 5. Red-Black Tree / Ordered Tree
Use for: VM region lookup, timer deadline ordering, ordered memory ranges.
- `bh_rbtree_t`
- `bh_rbtree_insert`
- `bh_rbtree_find_le`

### 6. Radix Tree / Trie
Use for: page-table-like sparse indexes, object ID namespace.
- `bh_radix_t`
- `bh_radix_insert`
- `bh_radix_lookup`

### 7. Reference Counting (`bh_refcount.h`)
Use for: physical pages, address spaces, kernel objects.
- `bh_refcount_t`
- `bh_refcount_inc`
- `bh_refcount_dec_and_test`

## 8. Classification Legend

| Status             | Meaning                                                                     |
| ------------------ | --------------------------------------------------------------------------- |
| Production-ready   | Safe for production kernel use with tests and documented semantics          |
| Needs hardening    | Exists but lacks tests, safety checks, clear API, or ownership rules        |
| Duplicate          | Similar implementation exists elsewhere and should be consolidated          |
| Deprecated         | Should not be used by new kernel code                                       |
| Missing            | No suitable kernel-owned primitive exists                                   |
| External candidate | Similar primitive exists outside kernel but should not be imported directly |

## 9. Current DS Inventory

| Primitive | Status | Current Location | Canonical Header | Current Users | Thread/IRQ Safety | Allocation Behavior | Test Coverage | Risks | Recommended Action |
| --------- | ------ | ---------------- | ---------------- | ------------- | ----------------- | ------------------- | ------------- | ----- | ------------------ |
| `bh_list` | Production-ready | `core/kernel/include/bharat/kernel/ds/` | `bh_list.h` | Scheduler, IPC | Caller-locked | Static/Intrusive | High (test_bh_list.c) | None | Keep as canonical intrusive list. |
| `bh_bitmap` | Production-ready | `core/kernel/src/ds/` | `bh_bitmap.h` | PMM, ID Allocator | Caller-locked | Static | High (test_bh_bitmap.c) | None | Keep as canonical bitmap. |
| `bh_ring` | Production-ready | `core/kernel/src/ds/` | `bh_ring.h` | IPC, Trace | Caller-locked | Static | High (test_bh_ring.c) | No atomics for multi-producer | Keep as simple ring; add atomic variant if needed. |
| `bh_id_allocator` | Production-ready | `core/kernel/src/ds/` | `bh_id_allocator.h` | Capabilities | Caller-locked | Static | High (test_bh_id_alloc.c) | Linear scan on full | Optimize find-first-clear in bitmap. |
| `bh_handle_table` | Production-ready | `core/kernel/src/ds/` | `bh_handle_table.h` | Handles | Caller-locked | Static | High (test_bh_handle.c) | None | Keep as canonical handle mgr. |
| `bh_range_tree` | Production-ready | `core/kernel/src/ds/` | `bh_range_tree.h` | VM Regions | Caller-locked | Static | High (test_bh_range.c) | O(N) insertion (sorted array) | Upgrade to RB-Tree for many regions. |
| `bh_seqlock` | Needs hardening | `core/kernel/src/ds/` | `bh_seqlock.h` | Timekeeping | Reader-lockless | Static | Low | Missing memory barriers | Harden in KDS-SEQLK-001. |
| `bh_rcu` | Needs hardening | `core/kernel/src/ds/` | `bh_rcu.h` | Capability Revocation | Lockless readers | Static | None (Stub) | Stub implementation only | Replace stub in KDS-RCU-001. |
| `radix_tree` | Needs hardening | `core/kernel/src/lib/ds/` | `radix_tree.h` | Capability Map | Lockless lookup | `kalloc` (Hidden) | None | Hidden dynamic allocation | Move/harden in KDS-RADIX-001. |
| `cuckoo_hash` | Needs hardening | `core/kernel/src/lib/ds/` | `cuckoo_hash.h` | Thread Lookup | Lockless lookup | Static | None | No host tests | Move/harden in KDS-CUCKOO-001. |
| `bh_refcount` | Production-ready | `core/kernel/src/ds/` | `bh_refcount.h` | PMM, VM Objects | Lockless (Atomics) | Static | High | No resurrection from zero | Keep as canonical refcount. |
| `wait_queue` | Needs hardening | `core/kernel/include/sched/` | `sched.h` | Scheduler, IPC | Scheduler-locked | Intrusive | Covered by sched tests | Embedded implementation | Extract in KDS-WAITQ-001. |

## 10. Duplicate and Overlap Findings

| Duplicate Primitive | Locations | Difference | Risk | Recommendation |
| ------------------- | --------- | ---------- | ---- | -------------- |
| `list_head_t` | `core/kernel/include/list.h` | Legacy Linux-style naming | Broken invariant tracking | Deprecate in KDS-LIST-CLEANUP-001. |
| `bh_list_node_t` | `core/kernel/include/bharat/kernel/ds/bh_intrusive_list.h` | Duplicate of `bh_list.h` | API fragmentation | Delete in KDS-LIST-CLEANUP-001. |
| `suggestion_queue_t` | `core/kernel/src/sched/sched_internal.h` | Ad-hoc sched queue | Non-reusable | Replace with `bh_ring` or `bh_list`. |
| `init_event_queue_t` | `core/services/core/init/` | Service-specific ring | Boundary violation (if in kernel) | Keep in service; if kernel needs it, use `bh_ring`. |

## 11. Missing Primitive Findings

| Missing Primitive | Needed By | Existing Candidate Elsewhere | Recommendation | Priority |
| ----------------- | --------- | ---------------------------- | -------------- | -------- |
| `bh_cpumask` | Scheduler, TLB | Raw `uint64_t` or `active_mask` | Add canonical `bh_cpumask` in KDS-CPUMASK-001. | P1 |
| `bh_mpsc_queue` | TLB Shootdown | None | Add bounded `bh_mpsc_queue` in KDS-MPSC-001. | P1 |
| `bh_registry` | Primitives, Personalities | Ad-hoc static arrays | Generic boot-time registry helper. | P2 |

## 12. Subsystem Mapping

| Subsystem      | Current DS Usage | Production Risk | Recommended Canonical DS | Follow-up Task    |
| -------------- | ---------------- | --------------- | ------------------------ | ----------------- |
| Scheduler      | `list_head_t`, `rb_node` | Legacy lists, External rbtree | `bh_list`, `bh_cpumask`, `bh_rbtree` | KSCHED-001        |
| PMM            | Custom freelist, Raw bitmap | Ad-hoc refcounting | `bh_list`, `bh_bitmap`, `bh_refcount` | KPMM-001          |
| TLB Shootdown  | `uint64_t` mask, Mailbox | Unbounded wait (legacy) | `bh_cpumask`, `bh_mpsc_queue` | KTLB-001          |
| Capability     | `bh_id_allocator`, CNode array | Handle safety | `bh_id_allocator`, `bh_handle_table` | KCAP-001          |
| VM/Aspace      | Custom RB-Tree, `vm_region` | Duplicate Tree logic | `bh_rbtree`, `bh_list` | KVM-001           |
| IPC/URPC       | `wait_queue_t`, Raw buffers | Embedded policy | `bh_ring`, `bh_waitq` | KIPC-001          |
| Fault Handling | Ad-hoc ID tracking | No isolation metadata | `bh_registry`, `bh_list` | KFAULT-001        |
| Telemetry      | Static Ring | Overflow silently | `bh_ring` (hardened) | KOBS-001          |

## 13. Canonicalization and Migration Strategy

### Phase 1 — Headers and wrappers
* Validate/retain `bh_refcount` as canonical; do not list it as missing.
* Plan movement of `radix_tree.h` and `cuckoo_hash.h` to `core/kernel/include/bharat/kernel/ds/` (KDS-RADIX-001, KDS-CUCKOO-001).
* Deprecate `list.h` and `bh_intrusive_list.h` (KDS-LIST-CLEANUP-001).

### Phase 2 — Host tests and hardening
* Add host tests for `radix_tree` and `cuckoo_hash` in `quality/tests/core/ds/`.
* Add stress tests for `bh_handle_table` and `bh_id_allocator`.
* Harden `bh_seqlock` memory barriers and tests (KDS-SEQLK-001).

### Phase 3 — Low-risk consumer migration
* Convert telemetry and trace to use hardened `bh_ring`.
* Convert simple id-allocations to `bh_id_allocator`.

### Phase 4 — Scheduler/MM/security migration
* Refactor PMM to use `bh_refcount`.
* Refactor Scheduler to use `bh_list` and `bh_cpumask` (KDS-CPUMASK-001).

### Phase 5 — Advanced algorithms
* Replace `bh_rcu` stub with epoch-based RCU-lite (KDS-RCU-001).
* Add bounded `bh_mpsc_queue` for per-core inbound work (KDS-MPSC-001).
* Extract `bh_waitq` from scheduler/IPCs (KDS-WAITQ-001).

## 14. Implementation Backlog

### Immediate Production Backlog
1. **PMM per-core magazines**: Per-core magazine cache + remote free queue.
2. **TLB shootdown**: Ring queue + request ID + ack bitmap + timeout.
3. **Capability table**: Generation-based handle table + rights bitmap.
4. **Scheduler runnable queues**: Intrusive list or priority bucket queues.
5. **VM region lookup**: Ordered tree/rbtree first; maple-tree-like structure later.

### Follow-up Task IDs
- **KDS-CPUMASK-001**: Add canonical `bh_cpumask` for scheduler/TLB CPU masks.
- **KDS-MPSC-001**: Add bounded `bh_mpsc_queue` for per-core inbound work.
- **KDS-SEQLK-001**: Harden `bh_seqlock` memory barriers and tests.
- **KDS-RCU-001**: Replace `bh_rcu` stub with epoch-based RCU-lite.
- **KDS-WAITQ-001**: Extract `bh_waitq` from scheduler/IPCs embedded wait queues.
- **KDS-RADIX-001**: Move/harden `radix_tree` under canonical DS layer.
- **KDS-CUCKOO-001**: Move/harden `cuckoo_hash` and add host tests.
- **KDS-LIST-CLEANUP-001**: Deprecate `list_head_t` and `bh_intrusive_list` duplicate wrappers.

### Preserved E3-X Roadmap Stories
- **E3-X-S1 — VM range index replacement strategy**: Replace linear or ad-hoc VM region lookup with a scalable range index (Maple-tree style or Red-Black).
- **E3-X-S2 — Kernel RCU/read-mostly primitive**: Introduce RCU-lite for read-mostly synchronization (non-preemptible first).
- **E3-X-S3 — XArray/radix-style object index**: Scalable sparse integer-key index for handles, capabilities, etc.
- **E3-X-S4 — Verified kernel hook prototype**: Typed hook table + static registration for safe observability.
- **E3-X-S5 — Storage metadata tree strategy**: B+tree or COW B-tree for filesystem layers.
- **E3-X-S6 — Memory-safe kernel abstraction layer**: Checked arithmetic, span/slice types, and ownership annotations for C.

## 15. Code-Agent Checklist

Before adding a new algorithm, answer:

1. **Is this mechanism or policy?** Mechanism lives in kernel/lib/drivers. Policy lives in services/stacks/personalities.
2. **Is this architecture-specific?** If yes, implementation belongs in `arch/`, contract in `hal/include/`.
3. **Is this reusable across kernel subsystems?** If yes, put it in `core/kernel/src/ds/`.
4. **Does it touch user-visible ABI?** If yes, put stable types in `uapi/`.
5. **Does it run on hot path?** Optimize for no locks, no heap, per-core data.
6. **Does it support small devices?** Use fixed-capacity or bounded growth.
7. **Does it have tests?** Host test for data structures is mandatory.

## 16. Rules for New Kernel Code

- Do not create private containers inside subsystems.
- Reuse canonical DS from `core/kernel/src/ds`.
- Generic DS must not contain subsystem policy.
- DS must have deterministic failure behavior.
- Cross-core DS must document memory ordering.
- IRQ-used DS must document IRQ-safety.
