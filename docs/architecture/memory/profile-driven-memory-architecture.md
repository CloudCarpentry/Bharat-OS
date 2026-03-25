# Profile-Driven Memory Architecture

## Context

Bharat-OS must scale from tiny MCUs to full server deployments. A monolithic memory architecture is insufficient, as embedded devices cannot afford the footprint of advanced virtual memory features they will never use, and server systems require sophisticated features like COW, NUMA, and demand paging. To solve this, the memory architecture is explicitly divided into a **Minimal Memory Core** and an **Advanced VM Tier**, governed by capability-based gating.

## Capability-Gated Build Features

The architecture avoids inferring memory features solely from a device profile (e.g., `EDGE`, `DATACENTER`). Instead, independent CMake capability flags explicitly dictate the available memory features:

*   `BHARAT_ENABLE_MMU`
*   `BHARAT_ENABLE_MPU`
*   `BHARAT_ENABLE_ADVANCED_VM`
*   `BHARAT_ENABLE_IOMMU`
*   `BHARAT_ENABLE_DMA_MAP`

These flags allow fine-grained control, such as building an `EDGE` profile with an MPU and IOMMU but without advanced VM features.

## Minimal Memory Core

The Minimal Memory Core is the foundational layer, always included in every build. It provides the essential mechanisms for managing physical memory and simple isolation.

**Components:**
*   PMM (Physical Memory Manager)
*   Page/Frame Allocator
*   Early Boot Allocation
*   Region Reservation
*   MPU / MMU-lite Abstraction (fixed mappings, region protection)
*   Basic Cache/TLB Hooks

## Advanced VM Tier

The Advanced VM Tier is conditionally included based on the `BHARAT_ENABLE_ADVANCED_VM` capability flag. It provides sophisticated virtual memory policies and features.

**Components:**
*   Address Spaces (`bharat_mm_aspace`)
*   VM Objects (`bharat_mm_objects`)
*   Demand Paging and Fault Resolution (`bharat_mm_fault`)
*   Hardware Page Table Translation Engine (`bharat_mm_pt`)
*   Advanced TLB Management (`bharat_mm_tlb`)
*   Copy-On-Write (COW)
*   NUMA-Aware Policies
*   Distributed / Cross-Core Memory Coordination

## Allocation Classes (Semantic Intent)

To support intelligent routing and observability without destabilizing the PMM API, memory allocations utilize `alloc_class_t` alongside traditional allocation flags.

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

This approach allows minimal profiles to collapse all classes into a single pool, while advanced profiles can route specific classes (e.g., `MEM_DMA`) to dedicated CMA regions or specific NUMA nodes.

## Build Presets

The build system utilizes intent-based CMake presets that compose these distinct capability flags.

**Preset Naming Convention:** `<role>-<mmu_class>-<build_type>`

**Examples:**
*   `tiny-mpu-debug`
*   `tiny-mpu-release`
*   `edge-mmu-lite-debug`
*   `gp-fullvm-release`

## Architecture Diagram

```mermaid
flowchart TD
    A[Build Presets (e.g., tiny-mpu-release, gp-fullvm-release)] --> B{CMake Capability Flags}
    B -->|BHARAT_ENABLE_ADVANCED_VM=ON| C[Advanced VM Tier]
    B -->|BHARAT_ENABLE_ADVANCED_VM=OFF| D[Minimal Memory Core]

    C --> E(Address Spaces, COW, Demand Paging, NUMA, TLB)
    D --> F(PMM, Frame Allocator, MPU-Lite, Fixed Mappings, Early Alloc)

    E --> F

    G[Memory Allocations] --> H{alloc_class_t}
    H -->|MEM_NORMAL, MEM_RT, MEM_PACKET| D
    H -->|MEM_DMA, MEM_SECURE| I[Specialized Pools / CMA]
```
