# Storage & Filesystem Roadmap (Code + Docs Alignment)

## Status legend

- **Done**: implemented and visible in current codebase.
- **In Progress**: partial implementation exists; contracts incomplete.
- **Planned**: documented target not yet implemented.

---

## Phase 0 — Baseline consolidation (Done)

- [x] Consolidate storage/filesystem architecture docs under `docs/architecture/storage/`.
- [x] Record repository-aligned architecture contract.
- [x] Record current hybrid migration state (kernel shim + service path).

Exit criteria:
- storage architecture readers can find canonical docs in one folder.

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
- [ ] convert remaining tests/consumers to clean service boundary usage.

Exit criteria:
- service path is authoritative for normal FS operations;
- kernel FS path reduced to explicit minimal mechanism compatibility.

---

## Phase 2 — Persistent filesystem maturity (Planned)

- [ ] keep RAMFS for early boot/tests, add persistent adapter track (FAT/littlefs/ext-like baseline by profile).
- [ ] add mount capability negotiation against backend feature descriptors.
- [ ] strengthen crash-consistency behavior (write ordering + sync semantics by policy).

Exit criteria:
- at least one persistent filesystem backend available and test-covered per non-volatile profile class.

---

## Phase 3 — Blob/object and policy hardening (Planned)

- [ ] replace blob backend placeholder with working object path.
- [ ] enforce storage-class restrictions (fs/block/blob) from capability policy.
- [ ] define staged write + commit semantics for object backend.

Exit criteria:
- blob backend is usable for read flow with capability enforcement.

---

## Phase 4 — Performance and scale features (Planned)

- [ ] deepen queue and completion model (virtio/NVMe multi-queue maturity).
- [ ] integrate NUMA locality behavior behind profile toggles where supported.
- [ ] add stress/fault-injection coverage for cache, flush, and recovery paths.

Exit criteria:
- measurable throughput/latency improvements under profile-appropriate workloads with repeatable test evidence.

---

## Ongoing doc-code alignment checklist

For every storage/filesystem PR:

1. Verify `docs/architecture/storage/*.md` reflects changed ownership/boundary behavior.
2. If interfaces in `kernel/include/fs`, `stacks/include/bharat/stacks/storage`, or `lib/fs` change, update architecture contract text in same PR.
3. Keep transitional claims explicit; do not document planned behavior as shipped.
