# Bharat-OS Memory Architecture: Capability Model

Bharat-OS supports a variety of target architectures with varying memory management hardware capabilities. Instead of forcing heavy virtual memory (VM) semantics onto all architectures, we have established a **canonical memory model capability matrix**.

The guiding principle is:
**Same architecture principles, different runtime depth**

We map targets to one of three canonical memory models:
- **`MEM_MODEL_MMU_FULL`**: 64-bit systems (x86_64, arm64, riscv64) with rich page-fault-driven virtual memory.
- **`MEM_MODEL_MMU_LITE`**: Constrained 32-bit systems with an MMU, where rich semantics are simplified or avoided.
- **`MEM_MODEL_MPU`**: Minimal constraint 32-bit targets without full paging, relying on static regions.

## Address Space Profiles

To bridge the gap between canonical memory hardware models and the running target's software memory semantics, Bharat-OS defines **Address Space Profiles** (`aspace_profile_t`). These profiles determine the "shape" and expected runtime behavior of an address space without conflating hardware truths and software policies.

The profiles include:
- **`ASPACE_PROFILE_FULL`**: Rich per-process address spaces with full page-granular protection.
- **`ASPACE_PROFILE_SPLIT`**: Shared kernel / constrained user split model.
- **`ASPACE_PROFILE_FLAT`**: Minimal protected flat mapping model. Note that "flat" is considered a runtime context style here, not a hardware memory model.
- **`ASPACE_PROFILE_REGION_ONLY`**: MPU-native protection context. No fake sparse paging assumptions or deep VM orchestration. Mandatory for `MEM_MODEL_MPU`.

These map logically to hardware profiles:
- `MEM_MODEL_MMU_FULL` typically uses `FULL` (or optionally `SPLIT`).
- `MEM_MODEL_MMU_LITE` typically uses `SPLIT` or `FLAT`.
- `MEM_MODEL_MPU` must use `REGION_ONLY`.

By querying `aspace_profile_get_current()`, core process/scheduler implementations can reject advanced VM assumptions on constrained profiles seamlessly.

### Mapping intent

- `mem_model_t` answers: what protection mechanism exists?
- `ASPACE_PROFILE_*` answers: what address-space creation semantics are allowed?

“Flat” is an address-space/runtime profile concept, not a hardware memory model.

### Enforcement rule

Constrained profiles must reject unsupported rich/full-VM creation semantics explicitly.
They must not silently proceed through the normal full-VM creation path.

Current conservative rule:
- `flags == 0` → basic creation
- `flags != 0` → treated as rich/full-VM semantics unless explicitly classified otherwise

Therefore:
- `ASPACE_PROFILE_FULL` allows rich creation semantics
- `ASPACE_PROFILE_SPLIT` is permissive in this PR
- `ASPACE_PROFILE_FLAT` rejects rich creation semantics
- `ASPACE_PROFILE_REGION_ONLY` rejects rich creation semantics

## The Memory Model Contract and Unsupported Operations

When an architecture does not support a specific feature (e.g. page-granular protection on an MPU), it **must fail explicitly**.

**The Rule:**
- An unsupported feature request MUST return an explicit error code (e.g., `-ENOTSUP`).
- It MUST NEVER return silent success (faking success).
- It MUST NEVER attempt to implement or emulate the feature poorly (e.g., pretending to handle page-faults on an MPU without genuine translation).

### Allowed Degradations

Each model determines what capability bits are exposed. We provide a queryable capability set: `mpa_caps_t` (Memory Protection Architecture Capabilities).

1. **`MEM_MODEL_MMU_FULL`**
   - Supports: `MEM_CAP_VIRT_ADDRSPACE`, `MEM_CAP_PAGE_MAP`, `MEM_CAP_PAGE_PROTECT`, `MEM_CAP_DEMAND_FAULT`, `MEM_CAP_SHARED_ASPACE`, `MEM_CAP_TLB_INVALIDATE`, `MEM_CAP_DMA_MAP`, `MEM_CAP_IOMMU`, `MEM_CAP_PER_CORE_PMM_CACHE`.
   - The full environment where deep VM services orchestrate process structures, NUMA allocations, and full TLB shootdown acks.

2. **`MEM_MODEL_MMU_LITE`**
   - Supports: `MEM_CAP_VIRT_ADDRSPACE`, `MEM_CAP_PAGE_MAP`, `MEM_CAP_PAGE_PROTECT`, `MEM_CAP_TLB_INVALIDATE`, `MEM_CAP_DMA_MAP`.
   - Typically skips heavy `MEM_CAP_DEMAND_FAULT` loops and deep `MEM_CAP_SHARED_ASPACE` orchestration to save footprint on 32-bit SMP systems.

3. **`MEM_MODEL_MPU`**
   - Supports: `MEM_CAP_REGION_PROTECT`.
   - Operates fully on region-based allocations. Features like `MEM_CAP_PAGE_MAP` are intentionally absent, and calls to map generic pages will explicitly fail. No "pretend VM" exists in the MPU layer.
   - **AI-Adjacent Constraints:** Explicitly rejects Tier P memory classes (e.g., `MEM_TENSOR_PINNED`, `MEM_SHARED_ACCEL`). Only Tier U (Universal) classes are permitted.

## AI-Adjacent Memory Class Tiering

Bharat-OS classifies AI-adjacent memory classes into two tiers for cross-profile truthfulness:

| Tier | Classes | Support Requirement |
| :--- | :--- | :--- |
| **Tier U (Universal)** | `MEM_TENSOR`, `MEM_MODEL_RO`, `MEM_SCRATCH_LOWLAT` | Mandatory on all memory models (MMU-full, MMU-lite, MPU). |
| **Tier P (Profile)** | `MEM_TENSOR_PINNED`, `MEM_STREAM_DMA`, `MEM_SECURE_MODEL`, `MEM_SHARED_ACCEL` | Optional; Rejected on `MEM_MODEL_MPU`. |

## Per-core Ownership Philosophy

The fundamental philosophy of per-core ownership remains in place regardless of the memory model:
- The **abstract authority path** stays identical: `fault/request -> aspace/protection context -> region/object -> arch/hal backend`.
- Heavy per-core structures (like large PMM cache magazines) are tied to `MEM_CAP_PER_CORE_PMM_CACHE` (optional on memory-constrained targets), but the strict ownership mechanics remain true everywhere.
