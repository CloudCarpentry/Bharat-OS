# Bharat-OS VFS & Storage Architecture

## 1. Goals
- **Hardware-Neutral:** Abstract the complexities of storage hardware (NVMe, virtio, eMMC) behind generic block and object interfaces.
- **Personality-Neutral:** The core kernel VFS exposes capability-based, generic object operations. POSIX (Linux), Android, and Windows NT semantics are layered on top via personality adapters.
- **Profile-Driven:** Allow compile-time and boot-time composition. RTOS profiles can use lightweight RAM-backed filesystems, while Datacenter profiles utilize async NVMe queues and remote object storage.
- **Capability-Native:** Eliminate global root/ACL concepts in the kernel. Mounts, file handles, and namespaces are governed exclusively by capabilities.

## 2. Non-Goals
- Implementing a monolithic Linux VFS clone inside the kernel.
- Hardcoding POSIX-specific behaviors (e.g., specific `errno` values or Linux-specific path resolution quirks) into the core path traversal logic.
- Building complex filesystems (like Btrfs or ZFS) before a stable, baseline POSIX and block IO path is proven.

## 3. The 5-Layer Storage Stack
The storage architecture in Bharat-OS is explicitly divided into 5 distinct layers:

### Layer 0: Hardware & HAL
Architecture-specific bus, interrupt, and DMA management.
*Examples: PCI-e initialization, arm64 MMIO routing, DMA memory mapping.*

### Layer 1: Device Class Drivers
Drivers that talk to specific storage hardware but implement a standard interface.
*Examples: NVMe driver, virtio-blk driver, eMMC driver, remote blob gateway.*

### Layer 2: Generic Block / Object Interfaces
The unified contract for asynchronous and synchronous I/O.
- **Block:** Request queues, elevator algorithms, flush/barrier/FUA semantics.
- **Object:** Put/Get semantics, URI routing, immutable read/staged write flows.

### Layer 3: Cache & FS Core (VFS Spine)
The capability-aware, neutral storage spine.
- Page Cache integration and writeback buffering.
- Generic inode/vnode objects (`vfs_node_t`).
- Mount tree (`vfs_mount_t`) and namespaces (`vfs_namespace_t`).
- Capability validation (path rights, storage-class rights).

### Layer 4: Personality Shims
Subsystems that translate the neutral VFS spine into ABIs expected by user applications.
- **Linux:** Maps generic open/read/write/rename to POSIX `openat`, `renameat2`, `stat`, and `errno`.
- **Android:** Adds Binder-mediated content providers and strict app sandboxing.
- **Windows:** Translates to NT-style handles, case rules, and reparse points.

## 4. Core Structures & Capability Model

Bharat-OS utilizes capabilities to manage authority across the entire stack.

- **`vfs_node_t`**: Represents a canonical storage object (file, directory, block device). It does *not* grant access; it contains an object ID and provenance metadata linking it to the capability system.
- **`vfs_mount_t`**: Represents a mounted filesystem instance. It carries a **mount capability** that dictates namespace visibility, path-rooted rights (R/W/X), and storage-class rights (e.g., whether this mount allows block access or only blob access).
- **`vfs_file_t`**: An open file description/handle. It holds a **live capability handle** that enforces specific rights (read, write, append) negotiated at open time, preventing ambient privilege escalation.
- **`vfs_namespace_t`**: A per-process or per-sandbox view of the mount graph, filtered by the capabilities assigned to the sandbox.

## 5. Profile Model

Subsystem features are activated based on the deployment profile:
- **RT/Safety Profile:** tmpfs/ramfs priority, deterministic allocation, minimal writeback buffering.
- **Edge/Mobile Profile:** Local filesystem priority, wear-aware block policies, per-app sandboxing namespaces.
- **Datacenter Profile:** Deep page caching, async block queues (virtio/NVMe), and remote blob storage integration.

## 6. Block vs. Blob Backends

- **Block Backends:** Utilize `block_device_t` and `block_request_t` for queue-based I/O. Supports hints like `sync`, `direct`, `buffered`, and `polling`.
- **Blob Backends:** Utilize `object_store_t` for URI-based addressing (e.g., `/blob/remote/bucket/key`). Designed for immutable reads and staged, transactional writes.

## 7. Phased Roadmap

### Phase 1: Block + Baseline POSIX (Current Focus)
- Implement Layer 2 generic block queues (`block.h`, `block.c`).
- Implement Layer 3 capability-aware mounts, namespaces, and file handles.
- Provide a stable, basic POSIX-style filesystem driver (e.g., ext2-like or FAT-like) over virtio-blk/NVMe.
- Add personality shims for standard POSIX file operations (`openat`, `stat`, `renameat2`, symlinks/hardlinks) mapping to the neutral core.

### Phase 2: Blob / Object Backend
- Introduce `VFS_BACKEND_BLOB` as a formal mount backend.
- Expose immutable read descriptors for remote URI paths.
- Implement staged writes via temp objects and atomic commit flows.

### Phase 3: Advanced Features (Deferred)
- Copy-on-Write (CoW) snapshots, deduplication, and encryption trees.
- Distributed replication and persistent memory (Optane) semantics.
- Complex caching algorithms and ML-driven I/O prediction.

## 8. Testing Strategy
- **Unit Tests:** Verify capability token validation on mounts and file descriptors without hardware drivers.
- **Integration Tests:** Mount a virtual ram-backed block device, create namespaces, and verify path resolution boundaries.
- **POSIX Conformance:** Run standard POSIX filesystem test suites against the Linux personality shim to ensure correct `errno` mapping and `openat`/`renameat2` semantics.