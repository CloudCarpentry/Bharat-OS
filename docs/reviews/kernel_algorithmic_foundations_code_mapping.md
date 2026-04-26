# Kernel Algorithmic Foundations — Code Mapping

## Purpose
Explain where each foundational primitive currently lives and what it will eventually replace.

## Current Scope
| Primitive | New Files | First Integration Target | Deferred Integration |
|---|---|---|---|
| ID Allocator | `core/kernel/src/ds/bh_id_allocator.c`<br>`core/kernel/include/bharat/kernel/ds/bh_id_allocator.h` | Capability Slot Allocation (`core/kernel/src/cap/`) | VM ID, Endpoint ID allocation |
| RCU Baseline | `core/kernel/src/ds/bh_rcu_stub.c`<br>`core/kernel/include/bharat/kernel/ds/bh_rcu.h` | RCU-safe Capability Lookups | Scheduler/IPC read-mostly paths |
| Range Tree | `core/kernel/src/ds/bh_range_tree.c`<br>`core/kernel/include/bharat/kernel/ds/bh_range_tree.h` | VM Region Lookup (`core/kernel/src/mm/vm/aspace/`) | IOMMU/DMA range management |
| Ring Buffer | `core/kernel/src/ds/bh_ring.c`<br>`core/kernel/include/bharat/kernel/ds/bh_ring.h` | Bounded Core MsgQ (`core/kernel/src/ipc/bh_core_msgq.c`) | TLB shootdown queue |
| Seqlock | `core/kernel/src/ds/bh_seqlock.c`<br>`core/kernel/include/bharat/kernel/ds/bh_seqlock.h` | Timekeeping / Stats | Shared Metadata read-mostly paths |

## Data Structure Placement Rules
- kernel/core primitives live under `core/kernel/src/ds/` and `core/kernel/include/bharat/kernel/ds/`
- do not create aspirational lib paths unless build-wired
- services/stacks migration is future work
- Host tests live in `quality/tests/core/ds/`

## Phase Mapping
### Phase 1 — ID allocator
Implement `bh_id_allocator` as a bitmap or table-backed allocator.
### Phase 2 — RCU baseline contract
Define `bh_rcu` headers and a functional stub (UP-safe/immediate).
### Phase 3 — Lockless/read-mostly registry preparation
Implement `bh_seqlock`, `bh_ring`, and `bh_range_tree` (sorted table).
### Phase 4 — Host tests
Comprehensive test suite in `quality/tests/core/ds/`.
### Phase 5 — Capability integration
Integrate `bh_id_allocator` into capability slot/object ID allocation in `core/kernel/src/cap/`.
### Phase 6 — VM region lookup abstraction
Add `vm_region_index.c/h` in `core/kernel/src/mm/vm/aspace/` as a canonical lookup path.

## Non-Goals
- no full maple tree implementation yet
- no full SMP RCU yet
- no capability semantic redesign
- no VM authority rewrite
- no folder migration in this PR
- no changes to `core/kernel/src/lib/ds/` (legacy/specialized structures)

## Follow-up Work
- Production RCU with grace-period engine.
- Augmented Interval Tree or Maple Tree for `bh_range_tree`.
- URPC registry cleanup using `bh_id_allocator`.
- Service-level integration for storage metadata.
