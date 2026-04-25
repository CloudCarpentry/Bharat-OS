# Execution Mode Build and Preset Guide

This document explains the CMake integration and the provided presets for configuring Bharat-OS with the Execution Mode and CPU Partitioning framework.

## CMake Options

The framework introduces several new cache variables to control its behavior:

### Core Framework Options
- `BHARAT_ENABLE_EXECUTION_MODE_FRAMEWORK`: Enables the core framework. Default is `ON`.
- `BHARAT_ENABLE_TEMPORAL_PARTITION_FALLBACK`: Enables temporal partitioning when spatial partitioning is not possible (e.g., 1-core or 2-core systems). Default is `ON`.

### Scheduler Class Options
- `BHARAT_ENABLE_SCHED_CLASS_FIFO_RT`: Enables the FIFO RT scheduler class. Default is `ON`.
- `BHARAT_ENABLE_SCHED_CLASS_DEADLINE_RT`: Scaffolding for deadline RT class. Default is `OFF`.
- `BHARAT_ENABLE_SCHED_CLASS_FAIR`: Enables the fair scheduler class. Default is `ON`.

### Configuration Variables
- `BHARAT_SYSTEM_PROFILE`: Defines the product domain (e.g., `AUTOMOBILE`, `MOBILE`, `APPLIANCE`, `DESKTOP`).
- `BHARAT_EXECUTION_MODE`: Defines the compute mode (e.g., `REALTIME`, `GENERAL_PURPOSE`, `MIXED_CRITICAL`).
- `BHARAT_LOGICAL_CPU_COUNT`: Overrides platform-discovered CPU count. Set to `0` to use automatic hardware discovery (default).

## Logical CPU Override

When `BHARAT_LOGICAL_CPU_COUNT` is set to a non-zero value, the execution mode framework will treat that value as the total number of active CPUs, bypassing the HAL platform discovery query. This is particularly useful for forcing fallback testing or configuring a specific runtime topology in a VM or simulator.

## Adding New Product Profiles

To add a new product profile:
1. Add the enum value to `bharat_system_profile_t` in `interface/uapi/system/execution_mode.h`.
2. Add the profile name string to the `BHARAT_SYSTEM_PROFILE` STRINGS property in `cmake/BharatExecutionMode.cmake`.
3. Create a CMake preset in `CMakePresets.json` combining the new profile with a valid execution mode and architecture.

## Adding a New Scheduler Class

To add a new scheduler class safely:
1. Define its mask in `bharat_sched_class_mask_t` in `interface/uapi/system/execution_mode.h`.
2. Implement a `sched_class_ops_t` instance for the new algorithm.
3. Call `sched_class_register(&my_new_class_ops)` during kernel boot initialization.
4. Update `cpu_partition_init` logic in `core/kernel/src/sched/cpu_partition.c` if the default mappings for the target profiles need to incorporate the new class.

## Presets

The repository includes several presets representing safe, verified configurations:

- `arm64-automobile-mix-1cpu`: 1-core temporal mapping.
- `arm64-automobile-mix-2cpu`: 2-core compressed mapping.
- `arm64-automobile-mix-4cpu`: 4-core spatial mapping.
- `arm64-mobile-gp-1cpu`: 1-core general purpose fallback.
- `arm64-mobile-mix-2cpu`: 2-core mobile mixed mapping.
- `arm64-mobile-mix-4cpu`: 4-core spatial mobile mapping.
- `riscv64-appliance-rt-1cpu`: 1-core realtime appliance.
- `x86_64-mobile-gp-2cpu`: 2-core x86 general purpose.
- `x86_64-desktop-gp-4cpu`: 4-core x86 general purpose spatial mapping.
