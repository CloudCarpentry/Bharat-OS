# Architecture Capability Matrix

This document defines the expected architecture capabilities for the runtime tiers.

## Capability Descriptions

- `ARCH_CAP_64BIT_VA`: The architecture supports a 64-bit virtual address space.
- `ARCH_CAP_SMP`: The architecture supports symmetric multiprocessing.
- `ARCH_CAP_MMU_FULL`: The architecture provides a full Memory Management Unit (MMU) capable of multi-level page tables and standard virtual memory semantics.
- `ARCH_CAP_DMA_COHERENT`: The architecture supports hardware-coherent DMA.
- `ARCH_CAP_ADV_IRQ_ROUTING`: The architecture provides advanced IRQ routing (e.g., MSI/MSI-X).
- `ARCH_CAP_IRQ_PROFILE_POLICY`: The architecture/HAL can enforce profile-driven interrupt policy at runtime.
- `ARCH_CAP_IRQ_HARVARD_SAFE`: The architecture/board supports explicit Harvard-safe interrupt vector and data placement rules.
- `ARCH_CAP_USERSPACE_HIGHHALF`: The architecture natively supports or is configured for a traditional user/kernel high-half split address space layout.
- `ARCH_CAP_HW_CRC`: The architecture provides hardware CRC acceleration instructions.
- `ARCH_CAP_SIMD_NET_CSUM`: The architecture provides SIMD acceleration for network checksums.

## Tier 1: Full Baseline

**Architectures:** `x86_64`, `arm64`, `riscv64`

Tier 1 represents the full feature set of Bharat-OS. These capabilities are **required**:

- `ARCH_CAP_64BIT_VA`
- `ARCH_CAP_SMP`
- `ARCH_CAP_MMU_FULL`
- `ARCH_CAP_DMA_COHERENT`
- `ARCH_CAP_ADV_IRQ_ROUTING`
- `ARCH_CAP_USERSPACE_HIGHHALF`
- `ARCH_CAP_IRQ_PROFILE_POLICY`

These capabilities are **optional** (may depend on the specific SoC or feature set):

- `ARCH_CAP_IRQ_HARVARD_SAFE`
- `ARCH_CAP_HW_CRC`
- `ARCH_CAP_SIMD_NET_CSUM`

## Tier 2: EDGE32 Compact Ports

**Architectures:** `arm32`, `riscv32`

Tier 2 represents the compact, edge-focused deployment. The mandatory capabilities are reduced to allow for easier bring-up and smaller memory footprints.

These capabilities are **required**:

- `ARCH_CAP_MMU_FULL`

These capabilities are **optional**:

- `ARCH_CAP_SMP`
- `ARCH_CAP_DMA_COHERENT`
- `ARCH_CAP_ADV_IRQ_ROUTING`
- `ARCH_CAP_USERSPACE_HIGHHALF`
- `ARCH_CAP_IRQ_PROFILE_POLICY`
- `ARCH_CAP_IRQ_HARVARD_SAFE`
- `ARCH_CAP_HW_CRC`
- `ARCH_CAP_SIMD_NET_CSUM`
