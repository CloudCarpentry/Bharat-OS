---
title: 'ADR-024: HMEM kernel mechanism and tensor SDK boundary'
status: Draft
owner: Team
last_updated: '2024-01-01'
tags:
- doc
see_also:
- none
---
# ADR-024: HMEM kernel mechanism and tensor SDK boundary

- Status: Accepted
- Date: 2026-08-12

## Decision

Bharat-OS defines HMEM as a capability-scoped heterogeneous memory contract
with a fixed-width Native UAPI descriptor and opaque generation-bearing
handle, rather than as a special allocator. The stable domain vocabulary
covers system, NUMA, pinned/DMA-visible, GPU/NPU device and shared memory, CXL,
PMEM, and HBM. Targets advertise only the domains they can truthfully back.
Cache synchronization and device mapping are semantic HAL operations; ISA
mechanics stay in architecture backends. Tensor metadata and ownership remain
exclusively in the SDK/runtime.

HMEM capabilities carry explicit create, CPU-map, device-map, pin, share,
migrate, query, and destroy rights. Accelerator submission and device DMA are
separate rights on their corresponding object types. CSpaces belong to
processes, not CPUs; an owner core temporarily mutates a process CSpace and
cross-core operations use fixed-width locators.

P0 supports aligned CPU-backed SYSTEM/legacy-shared memory, bounded CPU/device
mapping metadata, and truthful unsupported device mapping. Its lifecycle is
the implemented subset of create, place, CPU map, sync, and destroy. It does
not add compute queues, fences, device placement, graph operations, or
accelerator scheduling.

## Consequences

The same memory primitive can serve compute and non-compute consumers. HAL never authorizes capabilities. Device-local and noncoherent support cannot be claimed until discovery selects a safe backend. The transitional bounded registry must move to the canonical owner-local object/CSpace registry before distributed device mapping, without changing UAPI layout or stale-generation behavior.
