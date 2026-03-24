# ADR-010: Distributed Kernel Ownership Model

## Status

Accepted

## Context

While ADR-003 established the multikernel messaging spine (URPC) to replace global shared-memory locks with lockless messaging, the current implementation still exhibits shared-state SMP behaviors. Global OS objects (like physical memory frames, process/thread metadata, and address spaces) are often mutated directly across cores or rely on implicit core-0 authority (master-core drift).

This approach acts as a transport skeleton but lacks true distributed state semantics. If we continue treating kernel objects as anonymously shared blobs, scaling the system will result in complex race conditions, fragile cross-core synchronization, and poor NUMA performance. We need a concrete architectural model that explicitly defines who owns what data and how that data is safely mutated across cores.

## Decision

We will transition the Bharat-OS multikernel design from a shared-state SMP model toward a **distributed ownership model**. This evolution will be implemented in three distinct layers, prioritizing explicit communication over global consensus:

1.  **Transport Layer**: We will formalize the core-to-core messaging protocol in `multikernel.c` by introducing explicit message types, transaction IDs, acknowledgments, timeouts, and replay-safe caches. We will eliminate hardcoded notification-to-core-0 behaviors.
2.  **Ownership Layer**: Every global kernel object will be assigned an explicit "owner core" (e.g., via `mk_owner_for_frame`, `mk_owner_for_pid`). Ownership will initially be deterministic (based on NUMA node, hash, or creator core) rather than negotiated via consensus.
3.  **Consistency Layer**: Coordination protocols will be object-specific. For example, the Physical Memory Manager (PMM) will use a 2PC-lite protocol for cross-core frame reservations. Address-space modifications (VMM) will be owner-serialized, replacing global IPI broadcasts with targeted invalidation messages.

Global consensus algorithms (like Raft) will *not* be used for general kernel state. Consensus will be strictly reserved for highly specific, fault-tolerant control-plane operations (e.g., global namespace allocation) and only introduced after the ownership foundation is stable.

## Consequences

### Positive

*   **True Scalability**: By eliminating shared-memory mutation of remote objects, we remove hidden SMP bottlenecks and align with the original multikernel vision (ADR-003).
*   **Predictability**: Explicit ownership makes it clear which core has the authority to mutate a given object, simplifying reasoning about cross-core concurrency and race conditions.
*   **NUMA Optimization**: Tying object ownership to physical topology (e.g., NUMA nodes for physical frames) naturally optimizes memory access patterns.
*   **Decentralization**: Removing implicit core-0 bias creates a truly symmetric, decentralized kernel where any core can act as a monitor for its owned resources.

### Negative

*   **Increased Code Complexity**: Implementing transaction caches, timeout handling, and object-specific consistency protocols (like 2PC-lite for PMM) is significantly more complex than acquiring a spinlock.
*   **Performance Overhead for Remote Operations**: While local fast-paths remain fast, mutating a remote object now requires message passing and protocol overhead (e.g., a round-trip for frame reservation), which may increase latency for certain cross-core operations compared to an uncontended shared lock.
*   **Migration Effort**: Refactoring core subsystems (PMM, Scheduler, VMM, Capabilities) to respect the new ownership boundaries requires careful, staged migration to avoid destabilizing the current baseline.
