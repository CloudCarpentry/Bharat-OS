# Kernel Data Structures Inventory

## 1. Purpose

This document audits kernel-owned data structures and defines the canonical DS layer for Bharat-OS.

Canonical implementation root:

```text
core/kernel/src/ds
```

Canonical include root:

```text
core/kernel/include/bharat/kernel/ds
```

## 2. Classification Legend

| Status             | Meaning                                                                     |
| ------------------ | --------------------------------------------------------------------------- |
| Production-ready   | Safe for production kernel use with tests and documented semantics          |
| Needs hardening    | Exists but lacks tests, safety checks, clear API, or ownership rules        |
| Duplicate          | Similar implementation exists elsewhere and should be consolidated          |
| Deprecated         | Should not be used by new kernel code                                       |
| Missing            | No suitable kernel-owned primitive exists                                   |
| External candidate | Similar primitive exists outside kernel but should not be imported directly |

## 3. Inventory Table

| Primitive | Status | Current Location | Canonical Header | Current Users | Thread/IRQ Safety | Allocation Behavior | Test Coverage | Risks | Recommended Action |
| --------- | ------ | ---------------- | ---------------- | ------------- | ----------------- | ------------------- | ------------- | ----- | ------------------ |
| `bh_list` | Production-ready | `core/kernel/include/bharat/kernel/ds/` | `bh_list.h` | Scheduler, IPC | Caller-locked | Static/Intrusive | High (test_bh_list.c) | None | Keep as canonical intrusive list. |
| `bh_bitmap` | Production-ready | `core/kernel/src/ds/` | `bh_bitmap.h` | PMM, ID Allocator | Caller-locked | Static | High (test_bh_bitmap.c) | None | Keep as canonical bitmap. |
| `bh_ring` | Production-ready | `core/kernel/src/ds/` | `bh_ring.h` | IPC, Trace | Caller-locked | Static | High (test_bh_ring.c) | No atomics for multi-producer | Keep as simple ring; add atomic variant if needed. |
| `bh_id_allocator` | Production-ready | `core/kernel/src/ds/` | `bh_id_allocator.h` | Capabilities | Caller-locked | Static | High (test_bh_id_alloc.c) | Linear scan on full | Optimize find-first-clear in bitmap. |
| `bh_handle_table` | Production-ready | `core/kernel/src/ds/` | `bh_handle_table.h` | Handles | Caller-locked | Static | High (test_bh_handle.c) | None | Keep as canonical handle mgr. |
| `bh_range_tree` | Production-ready | `core/kernel/src/ds/` | `bh_range_tree.h` | VM Regions | Caller-locked | Static | High (test_bh_range.c) | O(N) insertion (sorted array) | Upgrade to RB-Tree for many regions. |
| `bh_seqlock` | Needs hardening | `core/kernel/src/ds/` | `bh_seqlock.h` | Timekeeping | Reader-lockless | Static | Low | Missing memory barriers | Add `smp_rmb/wmb` for cross-core. |
| `bh_rcu` | Needs hardening | `core/kernel/src/ds/` | `bh_rcu.h` | Capability Revocation | Lockless readers | Static | None (Stub) | Stub implementation only | Implement real epoch tracking (KDS-009). |
| `radix_tree` | Needs hardening | `core/kernel/src/lib/ds/` | `radix_tree.h` | Capability Map | Lockless lookup | `kalloc` (Hidden) | None | Hidden dynamic allocation | Move to `src/ds`, remove `kalloc` dependency. |
| `cuckoo_hash` | Needs hardening | `core/kernel/src/lib/ds/` | `cuckoo_hash.h` | Thread Lookup | Lockless lookup | Static | None | No host tests | Move to `src/ds`, add host tests. |
| `wait_queue` | Needs hardening | `core/kernel/include/sched/` | `sched.h` | Scheduler, IPC | Scheduler-locked | Intrusive | Covered by sched tests | Embedded implementation | Extract to generic `bh_waitq` (KDS-010). |

## 4. Duplicate/Overlap Findings

| Duplicate Primitive | Locations | Difference | Risk | Recommendation |
| ------------------- | --------- | ---------- | ---- | -------------- |
| `list_head_t` | `core/kernel/include/list.h` | Legacy Linux-style naming | Broken invariant tracking | Deprecate; migrate to `bh_list`. |
| `bh_list_node_t` | `core/kernel/include/bharat/kernel/ds/bh_intrusive_list.h` | Duplicate of `bh_list.h` | API fragmentation | Delete; use `bh_list.h` only. |
| `suggestion_queue_t` | `core/kernel/src/sched/sched_internal.h` | Ad-hoc sched queue | Non-reusable | Replace with `bh_ring` or `bh_list`. |
| `init_event_queue_t` | `core/services/core/init/` | Service-specific ring | Boundary violation (if in kernel) | Keep in service; if kernel needs it, use `bh_ring`. |

## 5. Missing Primitive Findings

| Missing Primitive | Needed By | Existing Candidate Elsewhere | Recommendation | Priority |
| ----------------- | --------- | ---------------------------- | -------------- | -------- |
| `bh_refcount` | PMM, VM Objects | Ad-hoc `atomic_t` usage | Add `bh_refcount` with saturating logic. | P0 |
| `bh_cpumask` | Scheduler, TLB | Raw `uint64_t` or `active_mask` | Add `bh_cpumask` for N-core scaling. | P1 |
| `bh_mpsc_queue` | TLB Shootdown | None | Add lock-free Multi-Producer Single-Consumer. | P1 |
| `bh_registry` | Primitives, Personalities | Ad-hoc static arrays | Generic boot-time registry helper. | P2 |

## 6. Subsystem Mapping

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

## 7. Canonicalization Plan

### Phase 1 — Headers and wrappers
* Move `radix_tree.h` and `cuckoo_hash.h` to `core/kernel/include/bharat/kernel/ds/`.
* Create `bh_refcount.h` and `bh_cpumask.h` stubs.
* Redirect `list.h` to `bh_list.h` with deprecation warning.

### Phase 2 — Host tests
* Add host tests for `radix_tree` and `cuckoo_hash` in `quality/tests/core/ds/`.
* Add stress tests for `bh_handle_table` and `bh_id_allocator`.

### Phase 3 — Migrate low-risk users
* Convert telemetry and trace to use hardened `bh_ring`.
* Convert simple id-allocations to `bh_id_allocator`.

### Phase 4 — Migrate scheduler/MM/security paths
* Refactor PMM to use `bh_refcount`.
* Refactor Scheduler to use `bh_list` and `bh_cpumask`.

## 8. Rules for New Kernel Code

* Do not create private containers inside subsystems.
* Reuse canonical DS from `core/kernel/src/ds`.
* Generic DS must not contain subsystem policy.
* DS must have deterministic failure behavior.
* Cross-core DS must document memory ordering.
* IRQ-used DS must document IRQ-safety.
