# Storage and Filesystem Architecture Contract (Bharat-OS)

## 1. Introduction & Guiding Principles

Bharat-OS implements a strict, profile-aware, and capability-governed multi-tiered architecture for its Virtual File System (VFS) and storage subsystems. The traditional monolithic VFS is decomposed across four distinct layers to ensure scaling from tiny MPU/MMU-lite devices, appliances, drones, automotive, mobile, edge, and cloud deployments.

### The Four Layers
1. **Kernel VFS Core (`kernel/`):** Provides *minimal mechanism only*. Handles object references (vnode/inode), path-walk core hooks, open/read/write/close/seek/metadata dispatch, capability checks at object boundaries, file descriptor/handle integration, notification hooks, and synchronization primitives. *Crucially, no policy (automount, sync strategy, media indexing) resides here.*
2. **Filesystem Service/System Layer (`services/system/filesystem/` & `stacks/storage/`):** The policy and orchestration layer. Owns mount policy, namespace rules, per-profile storage policy, integrity/scrub scheduling, sync/flush policy, user/session storage orchestration, and remote/network storage coordination.
3. **Storage Drivers/Backends (`drivers/block/`, `drivers/storage/`, & `stacks/storage/fs/`):** Separate actual backend implementations. Includes ramfs, devfs, tmpfs, flashfs (log-structured for small devices), block-backed local FS, network/distributed/cloud adapters, automotive partitioned storage, and read-only image FS for appliances.
4. **Userspace/UAPI Layer (`uapi/include/fs/`):** Exposes a clean, language-agnostic UAPI/SDK for file, directory, metadata, mount/query, and notification/watch operations, with optional POSIX compatibility.

---

## 2. Capability Matrix & Storage Profiles

Bharat-OS does not optimize first for "desktop POSIX completeness." It optimizes for a **filesystem capability matrix**, supporting various deployment classes through explicit storage profiles.

### 2.1 Profile Definitions

*   **A. Tiny / MPU / Appliance / Drone Controller (`FS_PROFILE_TINY_RO`, `FS_PROFILE_TINY_RW_LOG`, `FS_PROFILE_APPLIANCE`)**
    *   Tiny code size, static mount layout, mostly read-only or append-log.
    *   Predictable latency, flash-aware write behavior, minimal namespace complexity.
    *   Optional/no page cache.
*   **B. Mobile / Automotive / Appliance with Richer UX (`FS_PROFILE_MOBILE`, `FS_PROFILE_AUTOMOTIVE`)**
    *   Per-app/per-domain namespace control, journaling or crash consistency.
    *   Notification/watch support, better caching, removable media support.
    *   Secure partitioning, profile-aware storage classes.
*   **C. Cloud / Edge / Server (`FS_PROFILE_CLOUD`)**
    *   Scalable mount/namespace model, page cache/writeback sophistication, high IOPS backends.
    *   Richer metadata, quotas/snapshots, remote FS integration, observability.

---

## 3. Crash Consistency Tiers

*   **Tier 1:** Best effort / volatile only (ramfs/tmpfs/devfs).
*   **Tier 2:** Small-device log or copy-on-write metadata (flash/appliance/drone).
*   **Tier 3:** Journaled/block-backed consistency (mobile/automotive/general systems).
*   **Tier 4:** Advanced scalable/server features (snapshots/quotas/replication/integrity scrub).

---

## 4. Capability-Governed FS Access

The capability model is mandatory across services and IPC boundaries. Filesystem objects define:
*   Object type
*   Allowed rights (lookup, read, write, append, create, delete, execute, mount, remount, admin, watch, setattr)
*   Scope
*   Handle lifetime
*   Revoke behavior

---

## 5. Mount & Namespace Model

*   **Small-Device Mode:** Fixed mount table, no dynamic mount namespaces.
*   **Large-Device/Cloud Mode:** Global early boot namespace, system namespace, and per-process namespace (chroot-like environments), with read-only/read-write mounts and optional bind-like remaps.
