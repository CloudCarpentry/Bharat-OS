---
title: ASpace Lifecycle and TLB Generation
status: Draft
owner: Documentation Working Group
last_updated: 2026-04-25
tags:
  - docs
  - architecture
  - memory
see_also:
  - README.md
---
# ASpace Lifecycle and TLB Generation

## Lifecycle States
- `CREATED`: Initialized but not yet active.
- `ACTIVE`: Currently running on one or more CPUs.
- `DYING`: Destruction in progress; no new mappings allowed.
- `DESTROYED`: Fully released.

## TLB Generation
- Monotonic generation counter per address space.
- Incremented on every remote invalidation request.
- Used to track shootdown completion.

## Invariants
- Only `ACTIVE` or `CREATED` aspaces can receive mappings.
- Faults on `DYING` or `DESTROYED` aspaces are rejected (SIGSEGV/Kill).
