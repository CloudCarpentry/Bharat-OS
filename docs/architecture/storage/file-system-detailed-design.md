# Storage and Filesystem Detailed Design (Code-Aligned)

## 1. Scope and intent

This document maps storage/filesystem design to **current code** and records what is real, partial, or planned.

---

## 2. Implementation map

## 2.1 Filesystem service (`services/system/filesystem/`)

- `main.c`
  - selects profile at build/runtime macro level,
  - reads block geometry via `block_get_info`,
  - resolves storage policy via `storage_profile_resolve`,
  - initializes cache via `cache_init_with_profile`.
- `fd_mgr.c`
  - owns open-file table,
  - enforces capability-aware checks for open/read/write/close,
  - keeps `openat` in limited transitional mode (`AT_FDCWD`-style fallback only).
- `vfs_stub.c`
  - service-side driver registry helpers and path-prefix helpers.

## 2.2 Storage stack (`stacks/storage/`)

- `block/block.c`
  - validates request, routes to virtio-blk path for device 0,
  - handles flush request as explicit semantic.
- `cache/cache.c`
  - fixed-size global cache entries with profile-sized active capacity,
  - lookup + victim selection,
  - dirty block writeback and device flush in `cache_sync`.
- `profile.c`
  - profile resolver with device-size thresholds and arch clamping,
  - backend/cache/queue depth strategy,
  - string helpers for reporting.
- `fs/adapter.c`
  - lightweight adapter registration and lookup.
- `fs/ramfs/ramfs.c`
  - concrete page-backed RAMFS implementation with block-table growth.
- `fs/blob/blob_backend.c`
  - explicit compatibility placeholder returning service-required errors.

## 2.3 Kernel fs boundary (`kernel/src/fs/`)

- Contains compatibility units that intentionally return service-required status, preserving build/ABI continuity while policy migrates out of kernel.
- `kernel/include/fs/` remains important as shared contract surface for FS/VFS objects and operations.

## 2.4 Client boundary (`lib/fs/`)

- `fs_client` forwards operations to filesystem service symbols and acts as migration boundary for callers.

---

## 3. What is aligned now

1. **Profile-aware initialization exists in the service startup path.**
2. **Cache/writeback/flush scaffolding exists and is callable from service flow.**
3. **Filesystem adapter boundary exists (`adapter.c`) with concrete RAMFS backend present.**
4. **Driver side includes both virtio-blk and NVMe implementation baselines.**
5. **Capability-aware checks are present in active file operation path.**

---

## 4. Known mismatches and debt

1. **Hybrid ownership remains:** some VFS-related behavior is still available in multiple transitional locations.
2. **Service API coverage is incomplete:** no full syscall/IPC-complete `openat/read/write/stat/mkdir/unlink/rename` route set yet.
3. **Persistent FS adapters are not production-complete:** RAMFS is strongest concrete path today.
4. **Blob backend is contract placeholder only.**
5. **Kernel-to-service cutover is not fully complete for all consumers/tests.**

---

## 5. Documentation contract for contributors

When changing storage/filesystem code, contributors should keep these invariants:

- Do not move policy-rich namespace/mount behavior back into kernel internals.
- Keep driver code free of path/permission/POSIX policy logic.
- Route new profile decisions through `stacks/storage/profile.c` and consume them from service startup/config paths.
- Use `lib/fs/fs_client.*` as boundary for clients while transport evolves.

---

## 6. Relationship to roadmap

Delivery phases, priorities, and completion criteria are tracked in:

- [`roadmap.md`](roadmap.md)

This detailed design document should be updated alongside roadmap status changes whenever storage/filesystem behavior changes materially.
