# Storage and Filesystem Architecture Contract (Repository-Aligned)

## 1. Purpose

This document defines the **current authoritative architecture** for storage and filesystem behavior in Bharat-OS, aligned to code in:

- `services/system/filesystem/`
- `stacks/storage/`
- `drivers/block/` and `drivers/storage/`
- `kernel/src/fs/` and `kernel/include/fs/`
- `lib/fs/`

It intentionally separates:

- **What exists today** (implemented or stubbed), and
- **What is target architecture** (phased roadmap).

---

## 2. Layer model (current + target)

### Layer A — Driver layer (hardware-facing)

- `drivers/block/virtio_blk/`: block submission stub + init path.
- `drivers/storage/nvme/`: NVMe controller init/admin queue baseline.

**Contract:** no pathname parsing, no capability policy, no POSIX semantic ownership.

### Layer B — Storage stack primitives

- `stacks/storage/block/`: generic block request API (`READ`, `WRITE`, `FLUSH`).
- `stacks/storage/cache/`: bounded cache table with dirty tracking and flush path.
- `stacks/storage/profile.c`: profile resolver for backend, cache, queue depth, journaling/NUMA toggles.
- `stacks/storage/fs/adapter.c`: filesystem adapter registry.
- `stacks/storage/fs/ramfs/`: concrete in-memory filesystem backend.
- `stacks/storage/fs/blob/`: blob backend compatibility stub.

**Contract:** reusable policy/mechanism helpers, but not user ABI ownership.

### Layer C — Filesystem service

- `services/system/filesystem/main.c`: service bootstrap with storage profile selection + cache init.
- `services/system/filesystem/fd_mgr.c`: open/read/write/close table and capability-gated checks.
- `services/system/filesystem/mount_mgr.c`, `namespace_mgr.c`, `vfs_stub.c`: service-side namespace/mount and VFS helper scaffolding.

**Contract:** owns mount/namespace/file-descriptor policy and request dispatch behavior.

### Layer D — Kernel compatibility surface (transitional)

- `kernel/include/fs/*.h`: shared FS/VFS types and operation contracts.
- `kernel/src/fs/*.c`: currently mixed state; some files are compatibility shims returning `K_ERR_REQUIRES_FS_SERVICE`.

**Contract target:** kernel keeps only mechanism/ABI stubs required for safe transition; policy migrates to service.

### Layer E — Client boundary

- `lib/fs/fs_client.c`: fs client wrappers forwarding to filesystem-service symbols.

**Contract:** call-site stable entrypoints while IPC transport matures.

---

## 3. Current reality check (important)

The repository is in a **hybrid migration state**:

1. **Service path exists and performs real work for FD operations.**
2. **Kernel fs path still exists** and contains transitional compatibility shims in some units.
3. **Dual symbol patterns remain** (`vfs_*`-style behaviors in multiple places), which is accepted short-term but must be collapsed.
4. **Storage profile and cache wiring are present** in the service startup path.

This is expected for current phase, but documentation and code must explicitly acknowledge the hybrid state to avoid architectural drift.

---

## 4. Capability and authority model

Authority is capability-scoped. In current code paths, this is represented by:

- capability checks when opening and operating files (`caller_cap`, rights masks, target object checks).
- per-open file handle capability association in service FD tables.
- transitional dummy capability fallbacks in compatibility wrappers.

**Hard rule:** no ambient authority assumptions in new storage/filesystem work.

---

## 5. Driver taxonomy rule (`drivers/block` vs `drivers/storage`)

To keep current tree while reducing ambiguity:

- `drivers/block/*`: frontend request engines that consume normalized block requests.
- `drivers/storage/*`: transport/protocol-specialized storage controllers and queue managers.

Both remain valid now, but future docs/code should keep this split explicit.

---

## 6. Profile-driven policy baseline

The storage profile resolver currently derives:

- FS backend family intent (ramfs/littlefs/fat/ext4-like/xfs-like/blob)
- cache policy
- queue depth
- cache budget
- NUMA/journaling flags

Inputs are:

- app profile (`RT`, `EDGE`, `DATACENTER`)
- device size
- hardware architecture family

This baseline is already wired into filesystem service boot initialization.

---

## 7. Out of scope for current phase

Not yet treated as implemented architecture commitments:

- full userspace IPC protocol for complete POSIX file API surface,
- production persistent FS adapters beyond RAMFS-level baseline,
- complete blob/object implementation,
- full NUMA-local IO worker scheduling and queue pinning.

These remain roadmap items.
