# Bharat-OS Memory Architecture: Capability Model

Bharat-OS supports a variety of target architectures with varying memory management hardware capabilities. Instead of forcing heavy virtual memory (VM) semantics onto all architectures, we have established a **canonical memory model capability matrix**.

The guiding principle is:
**Same architecture principles, different runtime depth**

We map targets to one of three canonical memory models:
- **`MEM_MODEL_MMU_FULL`**: 64-bit systems (x86_64, arm64, riscv64) with rich page-fault-driven virtual memory.
- **`MEM_MODEL_MMU_LITE`**: Constrained 32-bit systems with an MMU, where rich semantics are simplified or avoided.
- **`MEM_MODEL_MPU`**: Minimal constraint 32-bit targets without full paging, relying on static regions.

## The Memory Model Contract and Unsupported Operations

When an architecture does not support a specific feature (e.g. page-granular protection on an MPU), it **must fail explicitly**.

**The Rule:**
- An unsupported feature request MUST return an explicit error code (e.g., `-ENOTSUP`).
- It MUST NEVER return silent success (faking success).
- It MUST NEVER attempt to implement or emulate the feature poorly (e.g., pretending to handle page-faults on an MPU without genuine translation).

### Allowed Degradations

Each model determines what capability bits are exposed. We provide a queryable capability set: `mpa_caps_t` (Memory Protection Architecture Capabilities).

1. **`MEM_MODEL_MMU_FULL`**
   - Supports: `MPA_CAP_VIRT_ADDRSPACE`, `MPA_CAP_PAGE_MAP`, `MPA_CAP_PAGE_PROTECT`, `MPA_CAP_DEMAND_FAULT`, `MPA_CAP_SHARED_ASPACE`, `MPA_CAP_TLB_INVALIDATE`, `MPA_CAP_DMA_MAP`, `MPA_CAP_IOMMU`, `MPA_CAP_PER_CORE_PMM_CACHE`.
   - The full environment where deep VM services orchestrate process structures, NUMA allocations, and full TLB shootdown acks.

2. **`MEM_MODEL_MMU_LITE`**
   - Supports: `MPA_CAP_VIRT_ADDRSPACE`, `MPA_CAP_PAGE_MAP`, `MPA_CAP_PAGE_PROTECT`, `MPA_CAP_TLB_INVALIDATE`, `MPA_CAP_DMA_MAP`.
   - Typically skips heavy `MPA_CAP_DEMAND_FAULT` loops and deep `MPA_CAP_SHARED_ASPACE` orchestration to save footprint on 32-bit SMP systems.

3. **`MEM_MODEL_MPU`**
   - Supports: `MPA_CAP_REGION_PROTECT`.
   - Operates fully on region-based allocations. Features like `MPA_CAP_PAGE_MAP` are intentionally absent, and calls to map generic pages will explicitly fail. No "pretend VM" exists in the MPU layer.

## Per-core Ownership Philosophy

The fundamental philosophy of per-core ownership remains in place regardless of the memory model:
- The **abstract authority path** stays identical: `fault/request -> aspace/protection context -> region/object -> arch/hal backend`.
- Heavy per-core structures (like large PMM cache magazines) are tied to `MPA_CAP_PER_CORE_PMM_CACHE` (optional on memory-constrained targets), but the strict ownership mechanics remain true everywhere.
