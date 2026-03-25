# Memory Operations (Memops) Layering

The memory operations layer in Bharat-OS defines how the kernel copies, zeros, and moves memory safely across different hardware execution environments (e.g., full MMU, MPU, no-MMU) and architectures (32-bit vs 64-bit).

## Layering Rules

The architecture strictly enforces one-way downward dependencies to prevent infinite recursion, especially in sensitive contexts like early boot, hardware fault handlers, and host tests.

### Tier 0: Raw Byte Primitives

The absolute lowest layer.

- **Examples:** `arch_memset_raw`, `arch_memcpy_raw`.
- **Rules:**
  - Must **never** call generic `memset()`, `memcpy()`, or `bzero()`.
  - Must **never** use allocators.
  - Safe for execution in early boot, traps/fault paths, and host tests.
  - Typically implemented as simple `volatile` loops or explicitly guarded architecture intrinsics to avoid compiler optimization cycles.

### Tier 1: Architecture-Accelerated Memops

- **Examples:** `arch_memset_fast`, `arch_memcpy_fast`.
- **Rules:**
  - Uses ISA acceleration (e.g., `rep movsb` on x86_64, or vector loads on ARM64) if available.
  - Uses word-width optimizations and cacheline assumptions where explicitly checked.
  - Must fall back to Tier 0 if fast paths are unavailable or unsafe (e.g., unaligned access faults on 32-bit RISC-V).
  - Must never call upward into generic wrappers.

### Tier 2: Environment-Aware Helpers

- **Examples:** `memops_zero_page`, `memops_copy_from_user`, `memops_zero_dma_buffer`.
- **Rules:**
  - These are policy/context-aware wrappers.
  - Checks page ownership, capabilities, DMA coherence, and permissions.
  - Must only call downward into Tier 1 or Tier 0.

## Architecture and Environment Concerns

### 32-bit vs 64-bit

- **32-bit:** Alignment traps are common. The word copy width is 4 bytes. Address arithmetic must carefully avoid accidental widening.
- **64-bit:** Larger word (8 bytes) and cacheline opportunities. Uses wider atomics/intrinsics. Must handle physical addresses correctly when they exceed virtual width assumptions.

### MMU / MPU / IOMMU Interaction

- **Full MMU:** Supports page-granular mapping, guard pages, lazy zeroing, and `copy_from_user` boundary checks.
- **MMU-Lite / MPU:** Operates on region-based mapping. Zero/copy helpers cannot assume full page-table semantics or demand paging recovery.
- **No-MMU:** Operates on a flat address model. Copy helpers are simpler, but safety shifts entirely to the capability model.
- **IOMMU (DMA):** If present, memops must be aware of DMA isolation, bounce buffers, and cache maintenance policies. Memory classes like `MEM_DMA_COHERENT` dictate how zeroing and copying behave under IOMMU enforcement.

## Forbidden Cycles

Never allow an architecture shim to call back into generic libc/kernel wrappers. For example, `arch_memset_scalar` invoking `memset()` creates a recursion loop that will crash the kernel stack during early initialization or host testing. Always delegate to Tier 0.
