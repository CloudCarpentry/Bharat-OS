# ADR: Build Presets for Memory Profiles

## Context

Bharat-OS targets a wide spectrum of hardware, ranging from embedded 32-bit MCUs (ARM Cortex-M, RV32) with no MMU, to large 64-bit servers with advanced VM and IOMMU support. The build system currently attempts to manage these configurations through scattered `#ifdef`s and broad profile names (e.g., `PROFILE_EDGE`, `PROFILE_DATACENTER`). This leads to combinatorial explosion and makes it difficult to verify specific architectural combinations (e.g., an edge device that *does* have an MMU vs. an edge device that *only* has an MPU).

## Decision

We will transition the build system to use intent-based CMake presets that compose distinct capability flags rather than relying on monolithic profile names.

Presets will be named using the convention: `<role>-<mmu_class>-<build_type>`.

Examples include:
* `tiny-mpu-debug`
* `tiny-mpu-release`
* `edge-mmu-lite-debug`
* `edge-mmu-lite-release`
* `gp-fullvm-debug`
* `rt-minimal-release`
* `dma-iommu-debug`

These presets will internally set a series of precise capability flags (e.g., `BHARAT_ENABLE_MMU=OFF`, `BHARAT_ENABLE_MPU=ON`, `BHARAT_ENABLE_ADVANCED_VM=OFF`, `BHARAT_ENABLE_IOMMU=OFF`).

## Rationale

1. **Explicit Capabilities:** By moving to capability flags (`BHARAT_ENABLE_ADVANCED_VM`, `BHARAT_ENABLE_MMU`), the CMake logic in `core/kernel/CMakeLists.txt` becomes much clearer. We conditionally link modules (like the VMM or IOMMU) based on their specific capability flag, not a vague profile string.
2. **Matrix Testing:** This approach allows us to easily create a test harness that iterates over specific presets (e.g., `tiny-mpu`, `mmu-lite`, `full-virtual-memory`) to ensure they compile and link correctly without missing symbol errors.
3. **Decoupling:** It decouples the *device profile* (the product intent) from the *hardware capability* (the MMU model). A small edge device may still require IOMMU for safety isolation, and this model supports that seamlessly.

## Consequences

- **Positive:** Developers can quickly spin up an environment targeting a precise set of memory constraints. Continuous Integration (CI) can easily validate the extremes of the configuration matrix. The CMake code is vastly simplified by checking boolean capabilities.
- **Negative:** The number of presets in `CMakePresets.json` will increase significantly.
- **Mitigation:** We will group presets logically and provide a `tools/test_memory_matrix.sh` script to automate the validation of the core preset combinations, reducing the manual burden on developers.
