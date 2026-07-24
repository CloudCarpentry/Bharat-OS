# Automotive Build Hygiene Follow-ups

This document tracks build hygiene issues discovered while implementing the automotive emulator and fast trace foundation. These issues were fixed with minimal patches to unblock the automotive profile but require deeper architectural cleanup.

## 1. Missing UAPI Headers
**Issue**: Several kernel-internal components were including UAPI headers that did not exist in the `interface/include` tree.
**Affected Components**: `core/kernel/src/core/fault_domain.c`, `core/kernel/src/sched/sched_thread.c`, `core/kernel/src/mm/footprint.c`, `core/kernel/src/sys/sys_constraints.c`.
**Minimal Fix**: Created transitional UAPI headers in `interface/include/bharat/uapi/system/`.
**Cleanup Needed**: Review these contracts with the respective subsystem owners to ensure they match the intended ABI.

## 2. Accidental Libc Dependencies in Kernel/Drivers
**Issue**: Many source files used `#include <string.h>` instead of the kernel-internal string library, causing cross-compilation failures for freestanding targets.
**Affected Components**: `core/kernel/src/ds/*`, `core/drivers/bus/*`, `core/drivers/devices/*`, `core/stacks/media/*`.
**Minimal Fix**: Mass-replaced `<string.h>` with `<lib/base/string.h>` or added include paths.
**Cleanup Needed**: Standardize a "no-libc" header policy and enforce it via `clang-tidy` or build-system checks.

## 3. Personality Table Linker Errors
**Issue**: `syscall_gate.c` had hard dependencies on all personality tables (Android, Windows), causing linker errors when those subsystems were disabled.
**Affected Components**: `core/kernel/src/trap/syscall_gate.c`.
**Minimal Fix**: Added `#if BHARAT_ENABLE_SUBSYS_...` guards around personality table lookups.
**Cleanup Needed**: Implement a dynamic personality registration system to remove hardcoded switches from the syscall gate.

## 4. Include Path Leaks
**Issue**: Some drivers and stacks (e.g., `bharat_headless_compositor`, `bharatos_driver_class`) were missing access to common stack headers.
**Affected Components**: `core/drivers/class`, `core/drivers/devices`, `core/stacks/media`.
**Minimal Fix**: Explicitly added `core/stacks/include` and `interface/include` to affected `CMakeLists.txt`.
**Cleanup Needed**: Refactor header organization to separate internal stack headers from public driver interfaces.

## 5. Broken Boot Mode Resolution
**Issue**: Early boot flow did not consistently resolve the boot mode from the command line before entering the runtime phase.
**Affected Components**: `core/kernel/src/main.c`, `core/kernel/src/kernel_boot.c`.
**Minimal Fix**: Added explicit `boot_mode_resolve` call in `kernel_main_common` and set `selected_mode` in `boot_info_t`.
**Cleanup Needed**: Unified boot mode resolution should be handled strictly by the bootloader adapters or a single early-init function.
