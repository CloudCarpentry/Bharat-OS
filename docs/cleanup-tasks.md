---
title: Bharat-OS Cleanup and Structural Alignment Tasks
status: Draft
owner: Documentation Working Group
last_updated: 2026-04-25
tags:
  - docs
  - general
see_also:
  - README.md
---
# Bharat-OS Cleanup and Structural Alignment Tasks

This document outlines the required cleanup tasks and folder structure improvements needed to bring the repository fully in line with the architectural boundaries defined in `docs/architecture/folder_structure.md`.

## 1. Loose Files to Remove (Deprecated / Redundant)

The following shell scripts in the root directory have been superseded by `tools/build.py` (as documented in `README.md`) and should be safely deleted to avoid confusion:

* `run_qemu_e2e.sh`
* `run_test.sh`
* `run_valgrind.sh`

*(Note: `build.sh` and `build.ps1` should be retained, as they act as the official, documented compatibility wrappers to `tools/build.py`.)*

## 2. Old Code to Remove

* **`core/services/legacy/`**: This entire directory (which contains the `core/services/legacy/net/` module) is explicitly marked as deprecated in its own `README.md`. It was the monolithic network stack placeholder and has been functionally superseded by `core/services/netmgr` and `core/services/netstack`. Removing it will reduce technical debt and build complexity.

## 3. Folder Structure Improvements

The architecture document outlines a strict, semantically sharp hierarchy. The current codebase has several deviations that need to be resolved.

### A. Top-Level Directory Discrepancies

1. **`staging/`**: The target structure expects a `staging/` directory at the root. Currently, staging code is nested under `staging/`. It should be moved to the root to keep the `core/kernel/` scope minimal and strictly focused on core mechanisms.
2. **`include/` (root)**: The HAL and Kernel are designed to own their own `include/` boundaries. Having a global `include/` directory is an anti-pattern under the current rules. Its contents (e.g., `include/bharat/` and `include/core/arch/`) should be migrated to `core/kernel/include/`, `corecore/hal/include/`, or `interface/uapi/` depending on their domain.
3. **`contracts/`**: This top-level folder should likely be integrated into `interface/uapi/`, `interface/idl/`, or `docs/`.
4. **`targets/` & `release/`**: Build matrices and artifacts should be consolidated under `tools/` or `staging/` to minimize root clutter.
5. **`user/`**: User-facing library code and applications should reside in `lib/` or `core/services/` as dictated by the target structure.

### B. The `core/services/` Hierarchy Reorganization

The `core/services/` directory is currently violating the architecture document by acting as a flat "catch-all" directory. The target structure dictates that `core/services/` must be categorized into strictly defined subdirectories: `core/`, `system/`, `security/`, `device/`, and `network/`.

The following migrations are required:

* **Move to `core/services/core/`**:
  * `power_mode`
  * `process_manager`
  * `memmgr`
  * `schedmgr`
  * `devmgr`
  * `coremgr`
  * `faultmgr`
  * `servicemgr`
  * `vm_manager`
* **Move to `core/services/system/`**:
  * `namesvc`
  * `telemetrymgr`
  * `storagemgr`
* **Move to `core/services/device/`**:
  * `sensor_hub`
  * `drivers` (assuming this refers to user-space driver service wrappers)
* **Move to `core/services/network/`**:
  * `can`
  * `netfast`
  * `netmgr`
  * `netstack`
* **Resolve `core/services/include/`**: Services should manage their headers internally or rely on `interface/uapi/` and `lib/` for shared contracts, rather than having an aggregated include directory inside `core/services/`.