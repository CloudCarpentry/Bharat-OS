# Storage and Filesystem Detailed Design (Current Implementation State)

## 1. Purpose

This document is **descriptive**: it captures current code reality, including partial implementations and transitional duplication.

It is not a normative boundary spec. Normative rules live in `vfs-and-filesystem-architecture.md`.

---

## 2. Current implementation map

## 2.1 Filesystem service (`services/system/filesystem/`)

- `main.c`
  - profile-aware startup path,
  - block geometry lookup (`block_get_info`),
  - policy/profile resolution (`storage_profile_resolve`),
  - cache initialization (`cache_init_with_profile`).
- `fd_mgr.c`
  - open-file table lifecycle,
  - capability-aware checks on open/read/write/close,
  - limited transitional `openat` behavior (fallback-focused subset).
- `mount_mgr.c`, `namespace_mgr.c`, `vfs_stub.c`
  - service-side mount/namespace and VFS helper scaffolding.

## 2.2 Storage stack (`stacks/storage/`)

- `block/block.c`
  - normalized request validation/routing,
  - read/write/flush request semantics.
- `cache/cache.c`
  - bounded cache entries,
  - dirty tracking and writeback,
  - explicit sync/flush path.
- `profile.c`
  - profile resolver (app profile + device + architecture signals),
  - backend/cache/queue-depth policy shaping.
- `fs/adapter.c`
  - adapter registry and lookup.
- `fs/ramfs/ramfs.c`
  - strongest concrete backend path currently in-tree.
- `fs/blob/blob_backend.c`
  - compatibility placeholder (not production blob backend).

## 2.3 Kernel FS boundary (`kernel/src/fs`, `kernel/include/fs`)

- Mixed transitional state remains:
  - `kernel/include/fs/*` provides shared contracts/types.
  - `kernel/src/fs/*` still contains compatibility-oriented behavior in some units.

## 2.4 Client boundary (`lib/fs`)

- `fs_client` forwards operations toward filesystem service paths and remains a migration boundary for callers.

---

## 3. Hybrid-state reality (explicit)

Current tree remains hybrid by design during migration:

1. Service path already performs meaningful FD operations.
2. Kernel FS transitional path still exists for compatibility continuity.
3. Duplicate/overlapping symbol ownership patterns still exist in some areas.
4. Service API surface is incomplete for a full POSIX-complete route set.

---

## 4. Known gaps and technical debt

1. **Ownership cleanup debt**
   - duplicate transitional ownership patterns still need collapse.
2. **Service request coverage gap**
   - not all operations (`openat/read/write/stat/mkdir/unlink/rename`) are complete end-to-end through service boundary.
3. **Persistent backend maturity gap**
   - RAMFS is currently the strongest concrete backend path.
4. **Blob backend maturity gap**
   - blob backend remains placeholder compatibility surface.
5. **Cutover completeness gap**
   - not all tests/consumers are fully migrated to clean service-owned paths.

---

## 5. Contributor guidance for implementation updates

When code changes storage/filesystem behavior:

- keep this document reality-based (shipped/partial/stubbed only),
- move normative boundary changes to `vfs-and-filesystem-architecture.md`,
- move future plans to `roadmap.md`,
- keep sandbox-policy semantics aligned with `sandbox-policy.md`.
