---
title: Storage & Filesystem Roadmap (Delivery Sequencing)
status: Draft
owner: Documentation Working Group
last_updated: 2026-04-25
tags:
  - docs
  - architecture
  - storage
see_also:
  - README.md
---
# Storage & Filesystem Roadmap (Delivery Sequencing)

## Status legend

- **Done**: implemented and visible in current codebase.
- **In Progress**: partial implementation exists; contracts incomplete.
- **Planned**: documented target not yet implemented.

---

## Platform gate policy (must hold)

Storage phases are gated by broader platform readiness:

1. **Capability baseline gate**
   - No Phase 2 persistent-FS expansion without baseline capability enforcement for service-owned FS policy.
2. **Service runtime maturity gate**
   - No Phase 3 blob/object activation without service runtime/event-loop maturity for reliable policy and request handling.
3. **Memory authority-path gate**
   - No Phase 4 performance/NUMA push without memory authority-path hardening sufficient for safe scale features.

---

## Phase 0 — Baseline consolidation (Done)

- [x] Consolidate storage/filesystem architecture docs under `docs/architecture/storage/`.
- [x] Separate canonical document roles: contract, current-state design, roadmap.
- [x] Add canonical sandbox policy under storage architecture folder.

Exit criteria:
- storage architecture readers can find canonical boundary, state, policy, and roadmap docs in one folder.

---

## Phase 1 — Stable service-driven baseline (In Progress)

### Current progress

- [x] filesystem service startup wired to block info + profile resolver + cache init.
- [x] capability-aware open/read/write/close path in service FD manager.
- [x] generic block API and cache sync/flush scaffolding.
- [x] fs client forwarding layer exists.

### Remaining

- [ ] complete service request surface for `openat/read/write/stat/mkdir/unlink/rename`.
- [ ] remove ambiguous duplicate transitional symbols/ownership patterns.
- [ ] convert remaining quality/tests/consumers to clean service boundary usage.

Exit criteria:
- service path is authoritative for normal FS operations;
- kernel FS path reduced to explicit minimal mechanism compatibility.

---

## Phase 2 — Persistent filesystem maturity (Planned; gated)

- [ ] keep RAMFS for early boot/tests, add persistent adapter track (FAT/littlefs/ext-like baseline by profile).
- [ ] add mount capability negotiation against backend feature descriptors.
- [ ] strengthen crash-consistency behavior (write ordering + sync semantics by policy).

Gate to enter Phase 2:
- capability baseline gate satisfied.

Exit criteria:
- at least one persistent filesystem backend available and test-covered per non-volatile profile class.

---

## Phase 3 — Blob/object and policy hardening (Planned; gated)

- [ ] replace blob backend placeholder with working object path.
- [ ] enforce storage-class restrictions (fs/block/blob) from capability policy.
- [ ] define staged write + commit semantics for object backend.

**Maturity note:** URI-backed mounts, remote providers, and staged-write commit/rename flows are Phase 3 targets only; they are not current runtime maturity claims.

Gate to enter Phase 3:
- service runtime maturity gate satisfied.

Exit criteria:
- blob backend usable for read path with capability enforcement;
- write path staged/commit semantics validated for declared scope.

---

## Phase 4 — Performance and scale features (Planned; gated)

- [ ] deepen queue and completion model (virtio/NVMe multi-queue maturity).
- [ ] integrate NUMA locality behavior behind profile toggles where supported.
- [ ] add stress/fault-injection coverage for cache, flush, and recovery paths.

Gate to enter Phase 4:
- memory authority-path gate satisfied.

Exit criteria:
- measurable throughput/latency improvements under profile-appropriate workloads with repeatable test evidence.

---

## Ongoing doc-code alignment checklist

For every storage/filesystem PR:

1. Update the right document type:
   - contract changes → `vfs-and-filesystem-architecture.md`
   - current-state changes → `file-system-detailed-design.md`
   - future sequencing changes → `roadmap.md`
   - sandbox-policy changes → `sandbox-policy.md`
2. If interfaces in `core/kernel/include/fs`, `core/stacks/include/bharat/core/stacks/storage`, or `lib/fs` change, update contract text in same PR.
3. Keep transitional claims explicit; do not document planned behavior as shipped.
