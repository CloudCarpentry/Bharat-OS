# Storage and Filesystem Detailed Architecture (Bharat-OS)

## 1. Introduction & Guiding Principles

Bharat-OS embraces a strict multi-tiered architecture for storage and file systems, adhering to the capability-oriented microkernel design philosophy. The traditional monolithic "Virtual File System" (VFS) is decomposed across four distinct layers to ensure isolation, flexibility across device profiles, and high-performance I/O (DMA/NUMA awareness) without bloating the kernel.

### The Four Layers
1. **Kernel (`kernel/`):** Provides *only* the low-level mechanisms. No filesystem policy, no pathname resolution, no heavy VFS. The kernel handles capability enforcement, IPC/uRPC transport, physical memory management (PMM), DMA-safe memory allocation, and fault delivery.
2. **System Services (`services/system/filesystem/`):** The VFS daemon. Owns mount policy, namespace rules, pathname traversal, file descriptor (session) handling, and sandbox storage policies.
3. **Storage Stack (`stacks/storage/`):** Reusable storage subsystem logic. Owns generic block request models, page/buffer caching, writeback engines, integrity frameworks, and filesystem protocol adapters (e.g., FAT, ext4-like).
4. **Drivers (`drivers/block/` & `drivers/storage/`):** Real device drivers (e.g., virtio-blk, NVMe, eMMC). Owns hardware probing, MMIO/registers, ring/queue submissions, DMA descriptor/SG list mapping, and interrupt/timeout handling.

---

## 2. Advanced Architectural Concepts

### 2.1 Direct Memory Access (DMA) and Zero-Copy I/O
To achieve near-production grade performance, the storage stack must minimize data copying across domain boundaries:
- **DMA-Safe Memory:** The `stacks/storage/cache` (Page/Buffer Cache) allocates I/O buffers from capability-granted, DMA-safe memory pools provided by the kernel PMM.
- **Zero-Copy IPC:** User applications issuing `read()` or `write()` calls negotiate shared memory windows (vmo/shm) via uRPC with `services/system/filesystem`.
- **Scatter-Gather (SG) Lists:** The generic block layer (`stacks/storage/block`) constructs SG lists referencing these pinned DMA buffers and passes them via capability-restricted IPC to the hardware drivers (`drivers/block/`).

### 2.2 NUMA Awareness
Modern Datacenter and Edge-Compute profiles feature Non-Uniform Memory Access (NUMA) topologies. The storage architecture accounts for this by:
- **NUMA-Local Page Caching:** The buffer cache allocates pages from the memory node closest to the CPU core executing the application thread.
- **I/O Queue Pinning:** Multi-queue block devices (like NVMe) map submission/completion queues to specific NUMA nodes. Interrupts and uRPC completion notifications are routed to the corresponding local cores to prevent cross-interconnect thrashing.
- **VFS Session Locality:** The `services/system/filesystem` daemon can spawn worker threads pinned to specific NUMA domains to handle high-throughput parallel I/O.

---

## 3. Profile-Driven Storage Features

Bharat-OS targets diverse environments. The storage stack components are conditionally compiled and configured based on the target profile:

| Feature / Subsystem | RT / Safety (Automotive/Medical) | Edge / Mobile (IoT/Drones) | Datacenter / Server |
| :--- | :--- | :--- | :--- |
| **Primary Backend** | eMMC / SPI NOR Flash | UFS / eMMC / SD Card | NVMe / Virtio-Blk / Network Blob |
| **Filesystem Type** | Deterministic (littlefs, FAT) | Wear-aware (f2fs, ext4-like) | High-throughput (xfs-like, btrfs-like) |
| **Cache Policy** | Minimal buffer cache (strict write-through) | Moderate page cache (write-back) | Deep, NUMA-aware page cache with async writeback |
| **I/O Queues** | Single queue, polling mode | Multi-queue, interrupt driven | Deep multi-queue, polling + interrupt hybrid |
| **Memory Allocation** | Static, pre-allocated DMA pools | Dynamic, reclaimable page cache | Transparent Huge Pages (THP), NUMA-bound |

---

## 4. Directory Structure and Responsibilities

Following the rigorous folder structure guidelines, the implementation is organized as follows:

### 4.1 Services: `services/system/filesystem/`
The authoritative VFS and Storage Policy Manager.
- **`mount_mgr.c`:** Manages the mount graph and resolves capability-anchored mount tokens.
- **`namespace_mgr.c`:** Handles per-sandbox path resolution and chroot-like environments.
- **`vnode_mgr.c`:** Orchestrates abstract virtual nodes (vnodes/dentries).
- **`fd_mgr.c`:** Maps per-process file descriptors to live `vnode` sessions and capability handles.
- **`ipc_dispatch.c`:** Handles uRPC messages (`openat`, `stat`, `mkdir`) from user apps and personalities.

### 4.2 Stacks: `stacks/storage/`
The reusable subsystem building blocks.
- **`block/`:** Generic block request queues, elevator algorithms, and I/O scheduler hints.
- **`cache/`:** Page cache, buffer cache, readahead logic, and writeback engine.
- **`fs/`:** Concrete filesystem format adapters (e.g., parsing superblocks, inodes, directory entries).
- **`integrity/`:** Checksums, journaling frameworks, and Merkle-tree validation (for secure profiles).

### 4.3 Drivers: `drivers/block/` & `drivers/storage/`
The pure hardware interface layer.
- **`drivers/block/virtio_blk/`:** Virtqueue initialization, descriptor chaining, and MMIO notifications.
- **`drivers/storage/nvme/`:** Admin/I/O queue pair creation, PRP/SGL construction, and doorbell ringing.
- **Constraints:** Drivers *never* parse paths, *never* check permissions, and *never* understand POSIX semantics. They only execute `block_request_t` arrays.

---

## 5. Implementation Roadmap

### Phase 1: Near-Production Baseline (Current Objective)
1. Relocate existing VFS stubs to `services/system/filesystem/`.
2. Establish the `stacks/storage/` framework (generic block API, basic buffer cache).
3. Implement a reference block driver (`drivers/block/virtio_blk/`) demonstrating DMA queue construction.
4. Wire uRPC IPC mechanisms to route a block read request from an app -> VFS -> Stack -> Driver.

### Phase 2: POSIX Personality & Caching
1. Implement a complete FAT32 or ext2 adapter in `stacks/storage/fs/`.
2. Connect `services/system/filesystem/` to the `compat/linux` personality to support real `openat`, `read`, `write`, `stat`.
3. Implement the NUMA-aware Page Cache in `stacks/storage/cache/` to buffer reads and aggregate writes.

### Phase 3: Advanced Sandboxing & Security
1. Enforce Path Capabilities: Validate that a sandboxed app's token allows writing to a specific subdirectory.
2. Implement Storage Class Restrictions (e.g., block raw `/dev/nvme0n1` access from untrusted containers).
3. Enable Secure Boot / Fast Boot profile integration (read-only verified rootfs with a volatile tmpfs overlay).

### Phase 4: Datacenter & Blob Scale
1. Implement multi-queue NVMe driver with CPU core pinning.
2. Introduce `VFS_BACKEND_BLOB` for URI-addressed object storage.
3. Asynchronous I/O via `io_uring`-style ring buffers mapped between the application and VFS.
