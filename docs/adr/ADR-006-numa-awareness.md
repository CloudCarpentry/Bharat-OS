# ADR-006: NUMA-Ready (Not NUMA-Complete) Architecture

## Status

Accepted

## Context

Bharat-OS targets massively parallel computing and Multikernel fabrics (Bharat-Cloud), naturally implying eventual deployment on Non-Uniform Memory Access (NUMA) systems. Standard operating systems (like Linux) employ incredibly complex policy frameworks to handle node topology, page migration, heuristics, and NUMA-balancing algorithms to ensure threads run near their physical memory banks.

Implementing this full stack on Day 1 is an "architectural sinkhole" that prevents the core system from achieving stability.

## Decision

**Bharat-OS Memory Management (v1) will be NUMA-Ready in its interfaces, but NOT NUMA-Complete in its policies.**

1. **Interfaces & Metadata**: We structure the C headers (`mm.h`, `numa.h`) from Day 1 to understand `memory_node_id`, node distance tables, CPU-to-node mapping, and capability-tagged memory provenance.
2. **Initial Implementation**: The actual allocating logic (e.g., `pmm.c`) assumes a single-node topology (like a standard QEMU VM). It silently ignores the `preferred_node` hints.
3. **Future Scaling**: When Bharat-Cloud scales to CXL multi-socket arrays, the memory-policy daemon (in user-space) can leverage the already-existing interface parameters to enact page migrations and NUMA balancing, without requiring a massive rewrite of the Ring-0 foundational logic.

## Consequences

### Positive

- **Simplicity**: We preserve boot velocity by implementing simple physical allocations (bitmaps) first.
- **Forward-Compatibility**: By passing location hints through the API now, we save ourselves from having to redesign the entire kernel object capability mapping tree later drastically.

### Negative

- **Minor Overhead**: Tracking `memory_node_id` in capability metadata slightly increases RAM overhead even on simple systems, though negligible.
