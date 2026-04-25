---
title: ADR: Allocation Classes
status: Draft
owner: Documentation Working Group
last_updated: 2026-04-25
tags:
  - docs
  - adr
see_also:
  - README.md
---
# ADR: Allocation Classes

## Context

In Bharat-OS, the physical memory allocator (PMM) currently relies on `uint32_t flags` for both fundamental allocation requirements (e.g., `PMM_ALLOC_ZERO`, `PMM_ALLOC_CONTIGUOUS`) and implicit usage intent. As Bharat-OS scales from tiny Cortex-M microcontrollers to large NUMA-aware cloud servers, this approach is insufficient. Different device profiles require the ability to route memory requests based on high-level semantic intent (e.g., routing DMA memory to a dedicated hardware pool, isolating secure packets, or pre-reserving real-time execution blocks).

Modifying all call sites across the kernel to pass a completely new structure would be destabilizing. We need a way to introduce semantic classification without breaking backward compatibility or complicating the minimal profiles.

## Decision

We introduce an `alloc_class_t` enumeration to explicitly capture the semantic intent of memory allocations alongside the existing `flags`.

```c
typedef enum alloc_class {
    MEM_NORMAL = 0,
    MEM_DMA,
    MEM_RT,
    MEM_SECURE,
    MEM_PACKET,
    MEM_LOWPOWER,
    MEM_PERSISTENT
} alloc_class_t;
```

We will provide a new primary allocation function `pmm_alloc_pages_ex` that accepts this class:

```c
void *pmm_alloc_pages_ex(size_t pages, alloc_class_t cls, uint32_t flags);
```

To maintain compatibility, existing functions like `pmm_alloc_pages` will become inline wrappers that default to `MEM_NORMAL`:

```c
static inline void *pmm_alloc_pages(size_t pages, uint32_t flags) {
    return pmm_alloc_pages_ex(pages, MEM_NORMAL, flags);
}
```

## Rationale

1. **Incremental Adoption:** Existing code continues to compile and function. Subsystems like the network stack or hardware accelerators can be updated iteratively to use `pmm_alloc_pages_ex` with `MEM_PACKET` or `MEM_DMA`.
2. **Profile-Specific Routing:** On tiny MPU profiles, all allocation classes can simply collapse into the single global heap. On large advanced VM profiles, `MEM_DMA` might route to a specialized CMA (Contiguous Memory Allocator) region or specific NUMA node.
3. **Observability:** By tagging allocations with semantic classes, we can trivially implement memory statistics (allocations, failures, and bytes used per class). This makes diagnosing out-of-memory errors in specific subsystems much easier.

## Consequences

- **Positive:** We gain the ability to enforce strict memory routing rules (e.g., "no GP memory can be used for RT paths"). Memory exhaustion diagnostics will be significantly improved.
- **Negative:** The PMM API surface area increases slightly. The underlying PMM implementation must now handle or safely ignore the `alloc_class_t` parameter.
- **Mitigation:** The memory statistics and routing logic will be wrapped in feature gates (e.g., `BHARAT_ENABLE_MEMORY_STATS`) so they can be compiled out entirely on minimal profiles, adding zero overhead where it matters most.
