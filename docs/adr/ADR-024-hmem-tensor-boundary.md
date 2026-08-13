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

Bharat-OS defines HMEM as a capability-scoped kernel memory object with a fixed-width Native UAPI descriptor and opaque generation-bearing handle. Cache synchronization and device mapping are semantic HAL operations; ISA mechanics stay in architecture backends. Tensor metadata and ownership remain exclusively in the SDK/runtime.

P0 supports aligned CPU-backed SYSTEM/SHARED memory, bounded CPU/device mapping metadata, and truthful unsupported device mapping. It does not add compute queues, fences, placement, graph operations, or accelerator scheduling.

## Consequences

The same memory primitive can serve compute and non-compute consumers. HAL never authorizes capabilities. Device-local and noncoherent support cannot be claimed until discovery selects a safe backend. The transitional bounded registry must move to the canonical owner-local object/CSpace registry before distributed device mapping, without changing UAPI layout or stale-generation behavior.
