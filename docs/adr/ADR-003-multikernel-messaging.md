# ADR-003: Multikernel Messaging Spine

## Status

Accepted

## Context

Traditional operating systems treat multi-core processors as a single monolithic block of shared memory protected by massive, complex lock hierarchies (Spinlocks, Mutexes, RCU). As core counts scale past 128+ physical cores, global cache coherency protocols saturate the memory bus. Hardware cache misses severely degrade performance.

## Decision

For Bharat-Cloud scaling, we adopted a **Multikernel Architecture**, heavily inspired by Barrelfish.
Instead of treating the entire CPU as shared memory, each core boots its own localized instance of the Bharat-OS microkernel. Global OS state (like a page table) is _replicated_, not shared.

When Core A needs to update a page table on Core B, it sends an explicit lockless asynchronous message over a shared-memory ring buffer (User-level Remote Procedure Call, URPC).

## Consequences

### Positive

- **Extreme Scalability**: Lock contention is eliminated. The OS scales linearly on enormous data-center chips without hitting the typical SMP (Symmetric Multiprocessing) bottleneck wall.
- **Heterogeneous Silicon**: The messaging spine can naturally extend over PCIe/CXL to pass URPC messages to GPUs or NPUs as if they were just different, slower cores.

### Negative

- **Memory Overhead**: Replicating state per-core consumes more physical RAM than a single shared structure.
- **Implementation Difficulty**: Maintaining eventual consistency across core-local page tables via messaging is significantly harder to initially implement than grabbing a spinlock.
