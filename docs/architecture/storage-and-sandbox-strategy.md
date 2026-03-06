# Storage & Filesystem Strategy (Advanced)

## Goals

Bharat-OS should expose a single, capability-safe storage model that can host:

1. **POSIX file systems** (ext-like, FAT-like, network FS).
2. **Raw block storage** (NVMe namespaces, virtio-blk, flash translation layers).
3. **Object/blob storage** (local content-addressed blobs and remote cloud buckets).
4. **Application sandboxes** that can strictly limit path visibility, mutation, and storage classes.

The design should keep Linux-personality compatibility while preserving microkernel isolation.

## Layered model

- **Layer 0: Device & transport**
  - block: NVMe, virtio-blk, eMMC/UFS
  - object transport: HTTP/S3-compatible gateways, cluster blob transport
- **Layer 1: Storage service**
  - provides normalized block and blob APIs to upper layers
  - supports QoS classes (latency, throughput, burst)
- **Layer 2: VFS core**
  - unified namespace and descriptor model
  - mount graph with per-mount capabilities
- **Layer 3: Personality adapters**
  - POSIX ABI adapter (`open/read/write/mmap/fcntl/stat`)
  - optional Linux personality compatibility shims
- **Layer 4: Sandbox policy engine**
  - per-process mount namespaces
  - path capabilities (read/write/exec/create/delete)
  - storage-class allowlists (block/blob/fs)

## Unifying block, blob, and filesystems

### Block storage

Treat block devices as first-class VFS nodes with explicit queue-depth and I/O mode hints:

- `sync`, `direct`, `buffered`, `polling`
- scheduler hints (`latency-first`, `throughput-first`)

This allows one policy engine to manage both `/dev/nvme0n1` and mounted filesystems.

### Blob storage

Represent blobs using URI-backed mount points:

- `/blob/local/<bucket>/<object-id>`
- `/blob/remote/<provider>/<bucket>/<key>`

Provide metadata-rich handles (etag, content hash, retention class).
For POSIX apps, expose a compatibility projection:

- read-only files as immutable objects
- write flows via staged temp objects + commit/rename transaction

### Multiple filesystem types

Use a driver registry with capability descriptors:

- supports journaling, snapshots, xattrs, ACLs, case-sensitivity, checksums
- advertises whether it can back POSIX hard links, sparse files, `mmap`

The VFS mount path negotiates required features from caller/sandbox policy against filesystem capabilities.

## POSIX-first compatibility path

Prioritize the minimum high-value POSIX set early:

- file descriptors, `openat*`, `stat*`, `read/write/pread/pwrite`, `fcntl`
- directories and `renameat2` semantics
- symlink and hardlink behavior
- robust errno mapping across native microkernel IPC

Then phase in advanced features:

- `mmap` consistency model
- file locks (`flock`/`fcntl`)
- async I/O (`io_uring`-like personality endpoint)

## Sandboxing model for filesystem restriction

Each sandbox context should carry filesystem policy in three dimensions:

1. **Namespace view**: what mount roots are visible.
2. **Path rights**: R/W/X/Create/Delete/Metadata per prefix.
3. **Storage class rights**: whether block, blob, tmpfs, network FS are permitted.

Recommended defaults:

- deny block-device raw access unless explicitly granted
- allow blob read for untrusted workloads, restrict blob write to designated buckets
- force `noexec,nodev,nosuid` on writable app mounts

## Innovative extensions

1. **Capability-anchored mount tokens**
   - mounts are created from signed capability tokens
   - token includes TTL, allowed path prefix, and storage-class constraints
2. **Intent-aware I/O contracts**
   - application can declare workload intent (`db-log`, `media-stream`, `ml-checkpoint`)
   - storage service maps intent to queue policy and caching strategy
3. **Dual-path persistence**
   - write critical metadata to local block journal + async blob checkpoint
   - improves recovery and fleet portability
4. **Policy-verifiable descriptors**
   - file descriptors carry sandbox provenance tags
   - kernel can prove an fd originated from an allowed namespace and mount policy

## Suggested implementation milestones

1. Extend VFS metadata to identify backend type (filesystem, block, blob).
2. Add filesystem driver capability registry.
3. Introduce sandbox filesystem policies with path-prefix rules.
4. Implement mount namespace + mount option enforcement.
5. Expand POSIX layer with `openat`/`statat` and errno parity tests.
6. Add blob mount prototype with immutable object reads.

## Success metrics

- POSIX test subset pass rate for file and directory operations.
- Measured policy enforcement coverage (blocked unauthorized path/storage accesses).
- p99 latency targets for block and blob reads under sandbox constraints.
- Reproducible crash recovery for mixed block+blob persistence mode.
