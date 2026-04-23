# ADR: Memory Core vs Advanced VM Split

## Context

Bharat-OS supports a multikernel architecture that must scale from highly capable server CPUs with advanced Virtual Memory Management (VMM), NUMA, Copy-On-Write (COW), and IOMMUs to deeply embedded Microcontroller Units (MCUs) that only possess an MPU or operate in a flat address-space profile context.

The kernel repository currently uses a monolithic approach where advanced memory features are often unconditionally included or conditionally `#ifdef`'d out based on a vague device profile enum, resulting in complex and error-prone `CMakeLists.txt` logic. If a 32-bit Cortex-M system is being built, the advanced VM code should not even be linked, nor should it contain `#ifdef`s trying to suppress features it fundamentally does not support.

## Decision

The memory subsystem will be explicitly split into a **Minimal Memory Core** and an **Advanced Virtual Machine (VM)** layer.

**Minimal Memory Core (Always Included):**
*   PMM (Physical Memory Manager)
*   Page allocator / frame allocator
*   Early boot allocation
*   Region reservation
*   MPU / MMU-lite abstraction (fixed mappings, region protection)
*   Basic cache/TLB hooks
*   Optional simple heap / static object pools

**Advanced VM (Conditionally Included via `BHARAT_ENABLE_ADVANCED_VM`):**
*   Address Spaces (`bharat_mm_aspace`)
*   VM Objects (`bharat_mm_objects`)
*   Demand Paging and Fault Resolution (`bharat_mm_fault`)
*   Hardware Page Table Translation Engine (`bharat_mm_pt`)
*   Advanced TLB Management (`bharat_mm_tlb`)
*   Copy-On-Write (COW)
*   NUMA-aware policies
*   Distributed / cross-core memory coordination
*   Full mapping lifecycle

The build system (`kernel/CMakeLists.txt`) will be refactored to conditionally link the Advanced VM components based on the `BHARAT_ENABLE_ADVANCED_VM` capability flag.

## Rationale

1.  **Code Size and Boot Speed:** By entirely omitting the advanced VM layer, embedded profiles (`tiny-mpu`, `rt-minimal`) gain substantial reductions in image size and faster boot times. They do not initialize complex VMM structures they cannot use.
2.  **Architectural Clarity:** The separation forces a clean boundary between the fundamental allocation of physical memory (PMM) and the sophisticated policies of virtual memory (VMM). The VMM must consume the PMM, never the other way around.
3.  **Testability:** We can compile a "no-VM" kernel and run host tests to prove that the core functions without undefined references to advanced VM symbols. This ensures the minimal core remains strictly isolated.

## Consequences

-   **Positive:** Minimal profiles become truly minimal. The architecture strongly enforces the boundary between mechanism (PMM) and policy (VMM). Testing the extreme configurations (MPU-only vs. Full MMU) becomes a simple matter of toggling CMake capability flags.
-   **Negative:** Any subsystem that previously assumed the existence of advanced VM features (like COW or demand paging) must now explicitly depend on `BHARAT_ENABLE_ADVANCED_VM` or provide a degraded fallback.
-   **Mitigation:** The capability flag will be derived from the selected device profile and hardware architecture, ensuring that profiles expecting full VM features receive them automatically.
