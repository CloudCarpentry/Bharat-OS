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

# Epic E3-X — Kernel Algorithmic Foundations

## Objective
Introduce production-grade kernel algorithms and verification foundations required for scalable VM, object indexing, read-mostly metadata, safe hooks, storage metadata, and memory-safe kernel coding.

---

## E3-X-S1 — VM range index replacement strategy

### Objective
Replace linear or ad-hoc VM region lookup with a scalable range index suitable for address-space regions.

**Recommended algorithm direction:**
Maple-tree style range index or augmented interval tree.
*Recommendation:* Maple-tree-inspired range index for VM regions, because it is optimized for range lookups and dense virtual address layouts. If implementation complexity is too high, start with an augmented red-black interval tree and keep the API compatible with a future Maple-style backend.

### Likely code areas
- `core/kernel/include/ds/`
- `core/kernel/src/ds/`
- `core/kernel/src/mm/vm/`
- `core/kernel/include/mm/`
- `quality/tests/host/`
- `docs/architecture/memory/`

### Tasks
1. Define `vm_range_index` API contract.
2. Compare interval tree vs Maple-style range tree.
3. Define lookup, insert, remove, split, merge behavior.
4. Define concurrency model: lock-based first, RCU-read later.
5. Define migration plan from existing VM region lookup.
6. Add benchmark requirements for region lookup scale.

### Acceptance criteria
- Roadmap defines chosen first implementation strategy.
- API supports lookup by address and range-overlap query.
- API does not expose internal tree representation.
- Migration plan avoids touching active VM logic in this story.

### Verification strategy
- Unit tests for insert/remove/overlap.
- Fuzz tests for random region maps.
- Benchmark: lookup performance across 10, 100, 1k, 10k regions.
- Invariant checks: no overlap unless explicitly allowed.

---

## E3-X-S2 — Kernel RCU/read-mostly primitive

### Objective
Introduce a minimal kernel read-mostly synchronization primitive for data that is frequently read and rarely updated.

**Recommended direction:**
RCU-lite first, full preemptible RCU later.
For Bharat-OS, start small:
- non-preemptible or cooperative RCU-lite
- per-core read-side nesting
- epoch/grace-period tracking
- deferred callback queue

### Likely code areas
- `core/kernel/include/sync/`
- `core/kernel/src/sync/`
- `core/kernel/src/sched/`
- `core/kernel/src/ipc/`
- `quality/tests/host/`

### Tasks
1. Define `rcu_read_lock()`/`rcu_read_unlock()` contract.
2. Define `synchronize_rcu()` baseline behavior.
3. Define `call_rcu()` deferred free contract.
4. Define UP behavior: read lock is mostly compiler/barrier discipline.
5. Define SMP behavior: grace period requires all cores to pass quiescent state.
6. Document RT constraints and non-goals.

### Acceptance criteria
- Roadmap clearly separates RCU-lite from full production RCU.
- UP and SMP behavior are both defined.
- RT-mode limitations are documented.
- No scheduler implementation is changed in this story.

### Verification strategy
- Host model tests for epoch transitions.
- Simulated multi-core quiescent-state tests.
- Negative test: object not reclaimed before grace period.
- Stress test plan for read-mostly object table.

---

## E3-X-S3 — XArray/radix-style object index

### Objective
Create a scalable object index for kernel objects such as handles, capabilities, VM objects, endpoints, devices, or file descriptors.

**Recommended direction:**
XArray/radix-tree inspired sparse integer-key index.

### Likely code areas
- `core/kernel/include/ds/`
- `core/kernel/src/ds/`
- `core/kernel/src/capability/`
- `core/kernel/src/ipc/`
- `core/kernel/src/mm/`
- `quality/tests/host/`

### Tasks
1. Define `kobject_index` API.
2. Support insert, lookup, remove, reserve ID, allocate next ID.
3. Define ID generation and stale-ID avoidance strategy.
4. Define RCU-read compatibility for future.
5. Define memory allocation constraints for small-device builds.

### Acceptance criteria
- Roadmap defines object-index API without coupling to capabilities.
- Sparse ID space support is required.
- Future RCU-read compatibility is documented.
- Small-device memory limits are considered.

### Verification strategy
- Unit tests for sparse inserts/removes.
- Duplicate ID rejection.
- Reuse-after-remove generation test plan.
- Fuzz test with random insert/remove/lookup.

---

## E3-X-S4 — Verified kernel hook prototype

### Objective
Introduce a safe kernel hook mechanism for observability, policy callbacks, and future verified extensions without arbitrary function-pointer chaos.

**Recommended direction:**
typed hook table + static registration + verifier rules.
Not dynamic eBPF yet. Start with verified static hooks.

### Likely code areas
- `core/kernel/include/verify/`
- `core/kernel/src/verify/`
- `core/kernel/include/hooks/` (if existing)
- `core/kernel/src/init/`
- `quality/tests/host/`
- `docs/architecture/kernel/`

### Tasks
1. Define hook type descriptor.
2. Define allowed hook points.
3. Define registration-time validation.
4. Define no-blocking / no-allocation / bounded-time rules.
5. Define future path to bytecode/eBPF-like verification.

### Acceptance criteria
- Hook prototype is typed and bounded.
- Invalid hook signature is rejected.
- Hook registration is static or early-boot only.
- No runtime dynamic loading in this phase.

### Verification strategy
- Compile-time signature tests where possible.
- Registration validation tests.
- Negative tests for invalid hook point.
- Documentation of safety rules.

---

## E3-X-S5 — Storage metadata tree strategy

### Objective
Define the metadata tree strategy for future production-grade storage and filesystem layers.

**Recommended direction:**
B+tree or copy-on-write B-tree depending on storage profile.

*For small devices:*
littlefs-style log/tree metadata strategy.

*For desktop/cloud:*
B+tree / extent tree / COW metadata option.

### Likely code areas
- `core/stacks/storage/`
- `core/stacks/storage/metadata/`
- `core/kernel/include/ds/`
- `core/kernel/src/ds/`
- `core/drivers/storage/`
- `docs/architecture/storage/`
- `quality/tests/host/`

### Tasks
1. Define storage metadata tree requirements.
2. Separate kernel DS primitives from storage policy.
3. Define extent-index requirements.
4. Define crash-consistency expectations.
5. Define small-device vs full-storage strategy.

### Acceptance criteria
- Roadmap does not put filesystem policy into kernel.
- Shared DS primitives can live under `core/kernel/ds` only if policy-free.
- Storage metadata policy belongs under `core/stacks/storage/metadata`.
- Small-device and desktop/cloud profiles have different strategies.

### Verification strategy
- Metadata tree model tests.
- Crash simulation plan.
- Extent lookup benchmark plan.
- Small-device memory budget analysis.

---

## E3-X-S6 — Memory-safe kernel abstraction layer

### Objective
Reduce kernel memory-safety bugs by introducing safer abstractions for slices, spans, checked arithmetic, bounded copies, and typed ownership.

**Recommended direction:**
C-safe abstraction layer with optional future Rust exploration.
Bharat-OS remains C-first for kernel baseline work. E3-X-S6 introduces memory-safe C abstractions such as checked arithmetic, span/slice types, bounded copies, ownership annotations, and static-analysis hooks. Rust may be evaluated later for selected isolated components, but this story is not a Rust rewrite.

### Likely code areas
- `core/kernel/include/ds/`
- `core/kernel/include/verify/`
- `core/kernel/src/ds/`
- `core/kernel/src/verify/`
- `core/kernel/include/lib/`
- `quality/tests/host/`
- `docs/dev/`

### Tasks
1. Define kernel span/slice type.
2. Define checked arithmetic helpers.
3. Define safe copy/move wrappers.
4. Define ownership annotation macros.
5. Define compiler-safety rules for freestanding kernel builds.
6. Define static-analysis/lint hooks.

### Acceptance criteria
- Roadmap defines memory-safe abstractions without forcing broad migration.
- Existing memops are not replaced in this story.
- Checked arithmetic and span APIs are proposed as opt-in first.
- Static-analysis path is documented.

### Verification strategy
- Unit tests for checked arithmetic overflow.
- Unit tests for span bounds checks.
- Compile-time tests for annotation macros where possible.
- Future CI lint plan.

---

## Implementation Status

| Primitive | Header | Implementation | Tests | Integrated With | Status |
|---|---|---|---|---|---|
| RCU stub | `bh_rcu.h` | `bh_rcu_stub.c` | `test_bh_rcu_stub.c` | none | baseline |
| Seqlock | `bh_seqlock.h` | `bh_seqlock.c` | `test_bh_seqlock.c` | none | baseline |
| ID allocator | `bh_id_allocator.h` | `bh_id_allocator.c` | `test_bh_id_allocator.c` | capability slot/object ID path | partial |
| Range tree/table | `bh_range_tree.h` | `bh_range_tree.c` | `test_bh_range_tree.c` | VM region lookup adapter | partial |
| Ring buffer | `bh_ring.h` | `bh_ring.c` | `test_bh_ring.c` | core msgq | baseline |
| Core MsgQ | `bh_core_msgq.h` | `bh_core_msgq.c` | none (uses ring tests) | future TLB/URPC | baseline |
