---
title: Epic E3-X — Kernel Algorithmic Foundations
status: Active
owner: Architecture Team
last_updated: 2026-05-20
tags:
  - architecture
  - kernel
  - roadmap
  - algorithms
---

# Bharat-OS Kernel Algorithmic Foundations

## 1. Purpose

Introduce production-grade kernel algorithms and verification foundations required for scalable VM, object indexing, read-mostly metadata, safe hooks, storage metadata, and memory-safe kernel coding.

## 2. Core Principle: Algorithm Placement Follows Ownership

Algorithm is not selected because it is advanced. Algorithm is selected because it matches the ownership, latency, memory, and security boundary of that subsystem.

Bharat-OS must not place algorithms only by convenience. Every algorithm must be placed according to ownership, latency criticality, architecture dependency, and policy weight.

- **Mechanism** belongs in the kernel, HAL, or drivers.
- **Policy** belongs in services, stacks, or personalities.

## 3. Current Repository Path Mapping

The canonical locations for kernel algorithmic foundations are:

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

## 5. Canonical Kernel Data-Structure Primitives

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

## 6. Naming Convention

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

## 7. Hot-Path Rules
- Avoid heap allocation.
- Prefer intrusive structures.
- Prefer per-core ownership.
- Avoid global locks.
- Name lock-aware APIs clearly: `_locked`, `_nolock`, `_try`, `_irqsave`.

## 8. Small-Device, MMU-Lite, and MPU-Only Constraints
- Provide fixed-capacity or compile-time bounded mode.
- Avoid unbounded dynamic growth.
- If a primitive requires an MMU, document the restriction.

## 9. Code-Agent Checklist

Before adding a new algorithm, answer:

1. **Is this mechanism or policy?** Mechanism lives in kernel/lib/drivers. Policy lives in services/stacks/personalities.
2. **Is this architecture-specific?** If yes, implementation belongs in `arch/`, contract in `hal/include/`.
3. **Is this reusable across kernel subsystems?** If yes, put it in `core/kernel/src/ds/`.
4. **Does it touch user-visible ABI?** If yes, put stable types in `uapi/`.
5. **Does it run on hot path?** Optimize for no locks, no heap, per-core data.
6. **Does it support small devices?** Use fixed-capacity or bounded growth.
7. **Does it have tests?** Host test for data structures is mandatory.

## 10. Immediate Implementation Backlog

The following priorities define the roadmap for applying these foundations:

1. **PMM per-core magazines**: Per-core magazine cache + remote free queue.
2. **TLB shootdown**: Ring queue + request ID + ack bitmap + timeout.
3. **Capability table**: Generation-based handle table + rights bitmap.
4. **Scheduler runnable queues**: Intrusive list or priority bucket queues.
5. **VM region lookup**: Ordered tree/rbtree first; maple-tree-like structure later.

---

## E3-X Stories (Preserved Roadmap)

### E3-X-S1 — VM range index replacement strategy
Replace linear or ad-hoc VM region lookup with a scalable range index (Maple-tree style or Red-Black).

### E3-X-S2 — Kernel RCU/read-mostly primitive
Introduce RCU-lite for read-mostly synchronization (non-preemptible first).

### E3-X-S3 — XArray/radix-style object index
Scalable sparse integer-key index for handles, capabilities, etc.

### E3-X-S4 — Verified kernel hook prototype
Typed hook table + static registration for safe observability.

### E3-X-S5 — Storage metadata tree strategy
B+tree or COW B-tree for filesystem layers.

### E3-X-S6 — Memory-safe kernel abstraction layer
Checked arithmetic, span/slice types, and ownership annotations for C.
