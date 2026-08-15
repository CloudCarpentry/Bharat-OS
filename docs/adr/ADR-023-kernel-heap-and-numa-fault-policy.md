---
title: 'ADR-023: Explicit Kernel Heap and NUMA Fault Policy'
status: Accepted
owner: Memory Working Group
last_updated: 2026-08-12
tags:
- doc
see_also:
- none
---
# ADR-023: Explicit Kernel Heap and NUMA Fault Policy

## Decision

Kernel heap selection is an orthogonal generated build capability named
`BHARAT_KMEM_ALLOCATOR`; kernel code must not infer it from a scheduler enum.
`AUTO` currently resolves to the existing SLAB backend for every profile.
Explicit `SLUB`, `SLOB`, and `RT_POOL` selections fail during configuration
until their implementations and qualification suites are present.

This fail-closed resolver is intentional. SLOB is not a hard-real-time or
safety allocator: first-fit search and fragmentation make its latency
unbounded. RT profiles require a separately reviewed bounded pool backend.

Threads retain the complete `numa_affinity_t`, including `LOCAL_PREFERRED`,
`BIND`, and `INTERLEAVE`. Demand faults pass the policy and fault virtual
address to PMM. `BIND` and the selected `INTERLEAVE` node are strict and never
fall back; `LOCAL_PREFERRED` may fall back. Invalid strict nodes fail allocation.
The node identifier remains 16-bit so `NUMA_NODE_ANY` cannot be truncated.

Anonymous fault zeroing uses the physical-map abstraction. A physical page
that cannot be mapped for initialization is released and the fault fails
closed. Fault addresses and object offsets are checked before allocation.

## Ownership and synchronization

The thread owner core owns its NUMA policy. Scheduler migration transfers that
policy with the thread; remote code must not directly mutate it. PMM retains
ownership of node-zone and per-core page-cache state and performs fallback only
when the supplied policy permits it.

## Qualification boundary and production roadmap

This ADR does **not** claim that the current heap or VM objects are production
complete. Enabling an advanced heap requires, in the same change:

* SLUB: owner-core fast paths, bounded remote-free queues, slab lifecycle and
  reclaim, corruption detection, NUMA creation, and 2/4-CPU SMP stress.
* SLOB: overflow-safe alignment, split/coalesce, invalid-free rejection,
  exhaustion/reuse and fragmentation tests; only for constrained non-hard-RT.
* RT_POOL: bounded allocation/free latency and pre-reserved fixed pools.
* VM objects: synchronized resident-page indexing, same-page reuse,
  destruction reclamation, shared backing, and PTE access translation.
* MEM-P0 kernel VA authority: one kernel address space, an SMP-safe VA arena,
  physical-page tracking, complete rollback, overlap protection, and `kvfree`
  reclamation. The current bump/list `kvmalloc` is not that authority.

Capability CSpace ownership is unchanged. Its transitional global table pool is
a separate P0: owner cores must mutate their own CSpaces and cross-core
grant/revoke must use the typed capability command path.

## Backends

The policy compiles for MMU, MMU-Lite, and MPU builds. Profiles without demand
paging do not execute the fault path; they still use the explicit heap resolver.
