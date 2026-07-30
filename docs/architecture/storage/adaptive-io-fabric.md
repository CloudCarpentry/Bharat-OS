---
title: Adaptive Storage & I/O Fabric Specification
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
# Adaptive Storage & I/O Fabric Specification

## 1. Scope & Non-Goals
The Bharat-OS Adaptive Storage and I/O Fabric provides a capability-safe, asynchronous, zero-copy-capable block storage path. It scales from tiny, constrained single-queue controllers up to high-throughput multi-queue NVMe systems, whilst preserving deterministic real-time and safety behaviors.

### Non-Goals
- Realizing physical hardware drivers (VirtIO, NVMe, SDHOST) beyond baseline scaffolding.
- Implementing a production sharded page cache or multi-tenant QoS.
- Directly communicating with hardware registers from VFS.

## 2. Layer Ownership
```
                     Applications / Services
               POSIX | VFS | DB | Object | OTA
                              │
                    Capability-gated async ABI
                              │
                 ┌────────────▼────────────┐
                 │ Storage Manager Service │
                 └────────────┬────────────┘
                              │
                 ┌────────────▼────────────┐
                 │ Adaptive Block Fabric   │
                 └──────┬───────────┬──────┘
                        │           │
              Cache/filesystem   Direct I/O
                        │           │
        ┌───────────────▼───────────▼──────────────┐
        │ Driver contract: submit / poll / cancel  │
        └──────┬────────┬────────┬────────┬────────┘
               │        │        │        │
            NVMe     VirtIO    SATA     SD/eMMC
```
- **VFS / Filesystem Service:** Coordinates path resolution and translates file operations to block streams.
- **Cache Layer:** Optional intermediate page/buffer cache sharding.
- **Asynchronous Block Fabric:** Standardized device-independent channel queuing, deadline tracking, and request merging.
- **Drivers:** Hardware-specific MMIO/PCI registers, PRPs, descriptors, and interrupt management.

## 3. Compile-Time Composition & Build Policy
I/O policies are configured via `BHARAT_IO_PROFILE` in CMake:
- `TINY`: Single fixed queue (depth 4), no cache, static allocation only.
- `EMBEDDED`: Shallow queues (depth 32), write-back cache, interrupt completion.
- `BALANCED`: Multi-queue support, write-back cache, hybrid completion.
- `THROUGHPUT`: Per-core queues, sharded NUMA cache, adaptive polling.
- `REALTIME`: Fixed queues, write-through cache, static allocation only, interrupt completion.

These options resolve into `<bharat/io_config.h>` for conditional compilation, stripping out unused code, caching layers, and driver backends.

## 4. Runtime Discovery & Registry
Storage devices are managed by the unified Registry. Multiple compiled drivers and runtime block devices coexist simultaneously (e.g., `nvme0`, `mmc0`, `virtio-blk0`, `memblk0`).
- Devices register their capabilities using `io_device_caps_t`.
- Discovery utilizes `block_device_find_by_role()` to decouple filesystems from hardcoded device IDs.

## 5. Request Lifecycle & State Machine
Every accepted request moves through a strictly defined state machine:
```
  FREE ──► QUEUED ────► IN_FLIGHT ────► COMPLETING ────► COMPLETED
             │                                              ▲
             └──► CANCELLED ────────────────────────────────┘
```
- **Invariant:** A successfully submitted request produces exactly one terminal completion (`SUCCESS`, `IO_ERROR`, `TIMEOUT`, `CANCELLED`, `DEVICE_REMOVED`, `NOT_SUPPORTED`).
- **Buffer Ownership:** The request descriptor is copied by the fabric on submission. Data buffers are borrowed by the fabric and pinned/inaccessible to the caller until the terminal completion signal is raised.

## 6. Cancellation & Timeout Semantics
- **Cancellation:** If the request is in `QUEUED` state, cancellation is guaranteed to succeed and generates a `CANCELLED` completion. If in `IN_FLIGHT` state, the cancellation is rejected or returns `IO_STATUS_NOT_SUPPORTED` / `BUSY`.
- **Timeout:** A timed-out request is distinct from an unexecuted write. Silently retrying timed-out writes is prohibited at the fabric level; retry policies reside in higher-level orchestration layers depending on idempotency.

## 7. Flush & Durability Barriers
Flush operations act as ordering barriers:
1. Writes submitted prior to a flush must reach persistent storage before the flush completes.
2. Writes submitted after the flush must not be bundled with the prior flush.
3. If a device has `flush_supported == false`, the fabric returns `NOT_SUPPORTED`.

## 8. Driver Integration Guide & Contract
Every storage driver must implement the `block_driver_ops_t` interface:
```c
typedef struct block_driver_ops {
    io_status_t (*probe)(void *driver_ctx);
    io_status_t (*get_caps)(void *driver_ctx, io_device_caps_t *out_caps);
    io_status_t (*open_channel)(void *driver_ctx, const io_channel_config_t *config, void **out_channel);
    io_status_t (*submit)(void *channel, const io_request_t *request);
    io_status_t (*poll_completions)(void *channel, io_completion_t *completions, uint32_t capacity, uint32_t *completion_count);
    io_status_t (*cancel)(void *channel, io_request_id_t request_id);
    ...
} block_driver_ops_t;
```

## 9. Current Maturity & Future Stories
- **Asynchronous Block Contract:** `Baseline`
- **Block Registry:** `Baseline`
- **Memblk Reference Driver:** `Baseline`
- **I/O Profile Build Policy:** `Baseline`
- **VirtIO Block Path:** `Scaffold` (returns explicit `NOT_SUPPORTED` error)
- **NVMe Path:** `Scaffold` (returns explicit `NOT_SUPPORTED` error)
- **SDHOST Path:** `Scaffold` (returns explicit `NOT_SUPPORTED` error)
- **Filesystem Decoupling:** `Partial`
