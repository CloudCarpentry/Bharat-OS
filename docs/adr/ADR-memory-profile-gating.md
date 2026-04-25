# ADR: Memory Profile Gating

## Context

Currently, the selection of memory management features in Bharat-OS is primarily driven by the `BHARAT_DEVICE_PROFILE` (e.g., `EDGE`, `DATACENTER`, `RTOS`). However, a device profile alone is insufficient to describe the memory capabilities required by a hardware platform. An `EDGE` device might be a Cortex-R with an MPU, an ARM64 processor with an MMU and IOMMU, or an x86_64 system.

Attempting to infer the memory feature set (e.g., whether to include advanced VM, COW, NUMA, or DMA mapping APIs) from the device profile leads to incorrect assumptions, such as assuming that all "small" profiles lack IOMMUs or advanced isolation mechanisms.

## Decision

We will introduce a set of independent CMake capability flags that dictate the memory features available, decoupled from the overarching device profile. The device profile, architecture, and board config will compute the defaults for these capability flags.

The key capability flags include:
*   `BHARAT_ENABLE_MMU` (ON/OFF)
*   `BHARAT_ENABLE_MPU` (ON/OFF)
*   `BHARAT_ENABLE_ADVANCED_VM` (ON/OFF)
*   `BHARAT_ENABLE_IOMMU` (ON/OFF)
*   `BHARAT_ENABLE_DMA_MAP` (ON/OFF)
*   `BHARAT_ENABLE_COW` (ON/OFF)
*   `BHARAT_ENABLE_DEMAND_PAGING` (ON/OFF)
*   `BHARAT_ENABLE_SHARED_MEMORY` (ON/OFF)
*   `BHARAT_ENABLE_NUMA` (ON/OFF)

These flags will be passed to the C preprocessor via `bharat_config.h` (e.g., `#define CONFIG_ENABLE_MMU 1`).

## Rationale

1.  **Granularity:** This allows fine-grained control over the kernel's memory subsystem. We can build an `EDGE` profile that has `BHARAT_ENABLE_MMU=ON` and `BHARAT_ENABLE_IOMMU=ON`, or an `EDGE` profile that has `BHARAT_ENABLE_MPU=ON` and `BHARAT_ENABLE_ADVANCED_VM=OFF`.
2.  **Safety and Isolation:** Even small edge devices may require an IOMMU for safety isolation (e.g., in automotive or medical profiles). Decoupling memory features from the device profile enables this flexibility without rewriting the profile logic.
3.  **Build System Simplicity:** The `core/kernel/CMakeLists.txt` file will simply check these boolean capability flags to determine which libraries and source files to include, rather than maintaining complex logic spanning architecture, board, and profile.

## Consequences

-   **Positive:** The memory architecture correctly reflects the underlying hardware capabilities and the product's policy intent. It prevents "bad assumptions" about what a specific profile implies regarding memory management.
-   **Negative:** The root CMake files will have an increased number of options to configure, potentially complicating manual builds.
-   **Mitigation:** We will provide curated `CMakePresets.json` configurations (e.g., `tiny-mpu`, `edge-mmu-lite`, `gp-fullvm`) that correctly assemble these capabilities, masking the complexity from the standard developer workflow.
