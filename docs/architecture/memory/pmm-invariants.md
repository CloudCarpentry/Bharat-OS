# PMM Invariants

This document defines the strict invariants for the Bharat-OS Physical Memory Manager (PMM).

## Core Invariants

1. **Machine-Readable State:** Every physical frame (PFN) in the system has exactly one machine-readable state defined in its `page_t` metadata structure.
2. **Free List Constraint:** Only FREE pages (`PMM_PAGE_STATE_FREE`) may appear on the buddy allocator's free lists.
3. **Reference Counting:**
    - A `ref_count == 0` implies the page is free and must be on the free list, unless its state is `RESERVED` or `DEVICE`.
    - PMM, not higher layers (like VM/COW), is the authoritative source of truth for frame refcounts.
4. **Pinning Semantics:**
    - Pinned pages (`pin_count > 0`) cannot transition to the free state, regardless of `ref_count`.
    - A page can only be freed when `ref_count == 0 && pin_count == 0 && state == ALLOCATED`.
5. **Contiguous Allocation:**
    - Contiguous physical allocations mark one head page and N-1 tail pages, or use equivalent exact-run metadata block tracking, ensuring exact runs of pages can be validated during the free path.
6. **Immutability:**
    - Zone assignments (`DMA32`, `NORMAL`, etc.) are immutable after system bootstrap.
    - NUMA node assignments are immutable after system bootstrap.
7. **Ownership Classes:**
    - Page-table pages and DMA pages are normal PMM objects. They are not special invisible allocations. They must have defined owner classes (`PMM_OWNER_CLASS_PAGETABLE`, `PMM_OWNER_CLASS_DMA`, etc.).