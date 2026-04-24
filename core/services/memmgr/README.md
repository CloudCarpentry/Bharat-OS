# services/memmgr

## Purpose
A user-space memory authority service above the kernel mechanisms. While the kernel owns page-table installs and hardware shootdown correctness, this service handles memory policies.

## Responsibilities
- **Region Policies**: Memory mapping definitions.
- **Paging Policy**: Management of memory eviction and paging.
- **COW Policy**: Management of Copy-on-Write policies.
- **Page Placement**: Deciding where pages reside physically.
- **Migration**: Page migration across NUMA and fabric domains.
- **Memory Pressure Handling**: Managing system memory constraints.
- **Hugepage Policy**: Rules regarding hugepages.
- **Tiered Memory Decisions**: Placement based on memory tiers (DRAM, NVM, etc.).

## Dependencies
- **May depend on:** `lib/runtime`, `lib/ipc`, standard C library headers.
- **Must not depend on:** `subsys/*`, `ui/*`, direct kernel-private headers.

## Immediate TODOs
- Connect to kernel memory mechanisms (e.g., fault queues).
- Implement basic COW policy and memory pressure handling.

## Status
Stub.
