---
title: Address-Translation Architecture Redesign (v2_fix)
status: Draft
owner: Documentation Working Group
last_updated: 2026-04-25
tags:
  - docs
  - archive
  - architecture
see_also:
  - README.md
---
# Address-Translation Architecture Redesign (v2_fix)

This document serves as the master plan for refactoring the Bharat-OS page-table and virtual memory subsystems into a robust, multi-tier address-translation architecture.

The goal is to move beyond the current "early baseline" scaffolding (which assumes a 64-bit MMU and relies on hardcoded `P2V` identity map macros) to a reliable, production-ready design that scales from rich server processors down to tiny embedded microcontrollers.

## The Three Execution Classes

The virtual memory subsystem must not be treated as "just improve page tables". It must support three distinct execution classes seamlessly, without breaking core kernel contracts:

1. **MMU-Full**: x86_64, arm64, riscv64 (Rich VA, TLB, Huge Pages)
2. **MMU-Lite**: arm32, riscv32 (Smaller VA, ASID limits, deterministic)
3. **MPU-Only**: Cortex-M / tiny edge targets (Region isolation only, no sparse paging)

## The 16-Point Master Plan

### 1. Separation of Concerns (The Most Important Design Decision)
`hal_pt.h` is too low-level to serve as a universal VM abstraction. We must separate the architecture into three sharp layers:
* **VM Policy Layer**: Regions, objects, fault policy, COW, lazy population.
* **Translation Backend Layer**: MMU page tables (`TRANSLATE_BACKEND_MMU`) or MPU regions (`TRANSLATE_BACKEND_MPU`).
* **TLB/Cache Sync Layer**: Local invalidation, remote shootdown, generation tracking.

### 2. Augmented Balanced Interval Tree for VM Regions
Replace simple linked lists with an augmented red-black interval tree keyed by start VA, with an augmented `max_end` in each subtree. This provides `O(log n)` lookups for page faults and overlap checks. For tiny EDGE32/MPU builds, an optional small sorted inline vector can be used to save memory.

### 3. Real Direct-Map Subsystem
Replace the naive `P2V(x)` and `V2P(x)` macros with a robust `physmap` API:
* `phys_to_virt_linear(paddr_t pa)`
* `virt_to_phys_linear(const void *va)`
* `virt_is_in_linear_map(const void *va)`
* `phys_is_linearly_mapped(paddr_t pa)`
This allows the kernel to properly handle sparse direct-map windows, high-half vs low-half layouts, and potential KASLR-like relocations.

### 4. Backend-Defined Page-Table Entries
Common code must stop reasoning directly about raw PTE width (e.g., `uint64_t entries[512]`). Introduce generic types:
* `pte_raw_t`: Arch-private raw descriptor.
* `vm_perm_t`: Common permission enum (e.g., RWX).
* `vm_memtype_t`: Common memory type (e.g., Device, Uncached).
* `map_level_t`: 4K / 2M / 1G or backend equivalent.

### 5. Slab-Backed Page-Table-Page Cache
Allocate PT pages from a dedicated `pt_page_cache` rather than generic PMM allocations to reduce latency and fragmentation.

### 6. Explicit Page-Table Walker
Introduce a shared `page_table_walk_result_t` object to prevent bugs from duplicated, ad-hoc page table walkers across different architectures.

### 7. Epoch-Based Remote TLB Invalidation
Model TLB shootdowns using generation/epoch tracking (`aspace->tlb_gen`, `cpu_seen_tlb_gen`) rather than fire-and-forget broadcast messages. Support both strict synchronous mode (for permission downgrades) and deferred mode (for benign upgrades).

### 8. Aggressive CPU Mask Tracking
Maintain per-address space `active_cpu_mask` to target TLB shootdowns strictly to CPUs currently running the affected address space.

### 9. Huge-Page Algorithm (Split Late, Merge Opportunistically)
Support large pages for MMU-Full targets. Implement base 4K pages mandatory, 2M huge pages first, and 1G optional. Split only on partial unmap, protect, COW fault, or subrange attribute change.

### 10. Reverse Mapping Hooks
Add reverse mapping metadata hooks to physical pages (refcount, pin count, owner/object class, reverse-map head) to enable future COW, revoke, and DMA pinning.

### 11. MPU Region Packing
For `PROFILE_MPU_ONLY`, do not fake sparse page tables. Use an MPU region packer to dynamically allocate and prioritize MPU slots based on size, constraints, and execute-never policy.

### 12. Finite-State Fault Path
Refactor the fault handler from a block of if/else logic into a formalized state machine or table-driven handler (decode -> lookup -> check perm -> backend repair -> TLB sync -> return).

### 13. Common Cache/DMA Attributes
Define `VM_MEM_NORMAL`, `VM_MEM_DEVICE`, `VM_MEM_DMA_COHERENT`, etc., and let individual architecture backends translate these into their specific bits (x86 PAT, arm64 MAIR, riscv PBMT).

### 14. 32-Bit Support Strategy
Do not chase full parity immediately. Introduce arm32 and riscv32 utilizing the stable common VM contracts, but gracefully degrade TLB richness, huge pages, and DMA cache maintenance. Do not merge MPU and MMU bring-up.

### 15. Prioritized Data Structures
* Must have: Augmented RB interval tree, Per-page metadata array, PT-page cache, Active CPU mask, Epoch counters, Common walk object.
* Avoid for now: Full maple-tree complexity, lock-free PT mutation, NUMA policies.

### 16. Execution Order (Phase 1 to Phase N)
1. **Phase 1:** Introduce `physmap` API, replace P2V/V2P macros, add `translate_backend_kind`, formalize `pte_raw_t`, `vm_perm_t`, and `vm_memtype_t`.
2. **Phase 2:** Implement the Augmented Interval Tree for VM regions.
3. **Phase 3:** Refactor arch PT code behind the authoritative explicit PT walker.
4. **Phase 4:** Add PT-page cache + teardown accounting.
5. **Phase 5:** Implement Epoch-based TLB generation contract and active CPU masks.
6. **Phase 6:** Bring up arm32/riscv32 MMU-Lite targets.
7. **Phase 7:** Add MPU-Only backend with Region Packer.
