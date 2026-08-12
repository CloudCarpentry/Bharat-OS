---
title: BHCF-001 Heterogeneous Memory
status: Experimental P0
owner: Kernel Memory and Compute Teams
last_updated: 2026-08-12
---
# BHCF-001: Heterogeneous Memory

HMEM is Bharat-OS's capability-scoped, device-neutral memory mechanism. It is not AI memory: media buffers, packet storage, DMA payloads, sensor frames, and tensors can all refer to the same object model. The ABI authority is `interface/uapi/bharat/memory/hmem.h`; its fixed-width handle never exposes a kernel pointer, physical address, page, or IOVA.

```text
Tensor / Media / Network / AI runtime
                 │
                 ▼
                HMEM
                 │
       ┌─────────┼─────────┐
       │         │         │
      CPU       GPU       NPU
       │         │         │
       └─────────┼─────────┘
                 │
            HAL / drivers
                 │
        cache / DMA / IOMMU
```

## Responsibility and lifecycle

The kernel validates descriptors, owns CPU backing storage, bounds ranges, enforces rights, tracks content generations and bounded mappings, and implements `LIVE → DYING → DEAD`. The current bounded registry is a documented lock-protected migration exception; handles carry slot generations and stale handles fail closed. Destruction rejects mapped objects, publishes `DYING` before releasing storage, and wipes storage when `ZERO_ON_FREE` is requested. There is no remote wait or cross-core message in P0.

HAL accepts already-authorized semantic sync/map requests. It performs no capability checks. Boot discovery may install one immutable backend operations table; absent device mapping returns `K_ERR_UNSUPPORTED` and never invents an address. The common coherent sync path is an ordering barrier. ISA cache mechanics remain below HAL; P0 emits no unqualified ARM or RISC-V cache instruction and therefore remains safe on MMU, MMU-Lite, and MPU targets.

## Rights and mapping

HMEM defines independent read, write, CPU-map, device-map, share, derive, pin, and migrate rights. Derivation must only attenuate these rights. CPU mapping checks requested access and permits one mapping in P0; double map, double unmap, mapping after destruction, and destruction while mapped fail closed. A four-entry device mapping table is deliberately bounded. Each entry records device identity, device address returned by a registered backend, access, and visible content generation.

Usage flags express caller intent; property flags express requested/established memory behavior. They are not hardware discovery. P0 supports CPU-backed SYSTEM and SHARED domains. DEVICE_LOCAL, PERSISTENT, and SECURE allocations return unsupported until a truthful allocator exists.

## Coherency and errors

Every sync range uses subtraction-based bounds checks so `offset + length` cannot wrap. Coherent memory uses a barrier fast path. Noncoherent memory routes to `hal_hmem_sync_range`; an installed architecture/driver backend must provide the maintenance. Invalid descriptors, permissions, stale generations, busy mappings, overflow, and absent backends retain distinct canonical errors.

Future IOMMU work will make device mappings owner-local transactions with rollback and quarantine on unsafe partial failure. Peer DMA and migration will require explicit visibility generations, acknowledgements, bounded retries, and no object lock held across remote completion.

## Security

Users cannot supply a physical address, IOVA, page pointer, kernel VA, or MMIO address. Authorization occurs above HAL. Unsupported domains and mappings fail closed. Execute access requires explicit usage and rights. Reserved ABI fields must be zero.

## Roadmap (documentation only)

```text
BHCF-P0-001   HMEM + Tensor SDK           ← this task
BHCF-P0-002   ENGINE object
BHCF-P0-003   FENCE/timeline primitive
BHCF-P0-004   asynchronous compute queue
BHCF-P1-001   IOMMU/device mapping
BHCF-P1-002   DMA/copy engine
BHCF-P1-003   Virtual NPU HMEM integration
BHCF-P1-004   accelmgr placement service
BHCF-P2-001   real GPU/NPU provider
BHCF-P2-002   tensor graph runtime
BHCF-P2-003   heterogeneous memory migration
BHCF-P3-001   KV-cache service
BHCF-P3-002   deadline/energy-aware accelerator QoS
```

## Benchmark evidence boundary

`BHCF-BENCH-001` measures deterministic allocation, mapping, tensor-view, copy,
and copied-byte counts plus non-gating elapsed time. QEMU evidence can establish
that HMEM/view handoffs avoid software copies and repeated allocation; it cannot
establish real GPU/NPU/DMA acceleration or hardware cache performance. Current
release-style benchmark profiles cover x86_64, ARM64, and RISC-V 64. They use
virtual CPUs and RAM and intentionally make no physical accelerator claim.
