# Bharat-OS Boot Experience Baseline & Audit

**Date**: 2026-10-10  
**Branch**: `feature/unified-boot-experience`  
**Base Commit**: `origin/developer`  

## 1. Executive Summary

This document establishes the architecture baseline and component audit for the **Unified Cross-Architecture Boot Experience** in Bharat-OS across `x86_64`, `ARM64`, `ARM32`, `RISC-V64`, and `RISC-V32`.

The goal is to transition the operating system from a single-target desktop showcase to a cohesive multi-architecture boot, branding, diagnostics, and graphical presentation without introducing kernel dependencies on the display pipeline or violating microkernel / per-core layer separation.

---

## 2. Component & Subsystem Audit

### 2.1 Boot Event & Telemetry Interface
- **Current UAPI**: `interface/include/bharat/uapi/boot/boot_events.h`
  - Defines `bh_boot_stage_t` enum (`EARLY`, `HAL`, `SECURITY`, `MEMORY`, `SCHEDULER`, `SERVICES`, `USERSPACE`, `READY`).
  - Defines single `bh_boot_event_t` with stage, percent, status, and label.
- **Kernel Mechanism**: `core/kernel/src/boot/boot_events.c`
  - Current implementation passes directly through to `boot_gui_update_progress(percent, label)`.
  - Missing bounded history buffer, timestamps, component identifiers, error codes, and snapshot query interface.
- **Baseline Need**: Extend to a non-allocating, lockless/bounded-spin ring buffer with monotonic HAL timestamping and snapshot retrieval.

### 2.2 Boot Display Service (`boot_displayd`) & Early UI Stack
- **Service Location**: `core/services/system/boot_displayd/`
- **Renderers**:
  - `core/stacks/ui/lcd/tiny_ui.c`: Lightweight framebuffer renderer with page-based layout (`splash`, `diagnostics`, `recovery`).
  - `experience/apps/gui_showcase/`: LVGL-based desktop showcase application.
- **Current Limitation**:
  - `boot_displayd` relied on mock event progression arrays instead of live kernel/service event queries.
  - Colors and brand text were hardcoded in `shell_ui.c` and `tiny_ui.c`.
- **Baseline Need**: Modular theme contract (`bh_ui_theme_t`) and live boot event consumer in both `tiny_ui` and LVGL splash/diagnostics modes.

### 2.3 Targets & Platform Matrix
- **Target Definitions**: `delivery/targets/qemu/` (39 YAML target profiles).
- **Core Headless Targets**:
  - `x86_64_desktop_headless.yaml`
  - `arm64_desktop_headless.yaml`
  - `arm32_mmu_lite_headless.yaml`
  - `riscv64_desktop_headless.yaml`
  - `riscv32_mmu_lite_headless.yaml`
- **Core GUI Showcase Targets**:
  - `x86_64_desktop_gui.yaml` / `x86_64_showcase_gui.yaml`
  - `arm64_desktop_gui.yaml`
  - `riscv64_desktop_gui.yaml`
- **Current State**:
  - `x86_64`: Full GUI and Headless boot verified.
  - `ARM64`: Headless boot verified; GUI pipeline supported via VirtIO-GPU.
  - `RISC-V64`: Headless boot verified; GUI pipeline supported via VirtIO-GPU.
  - `ARM32`: PMM boot under active investigation; operates in Headless mode.
  - `RISC-V32`: Headless boot verified; operates in Headless / Compact mode.

### 2.4 QEMU Matrix Runner & Verification Tooling
- **Runner**: `tools/run_qemu_matrix.py` wrapping `tools/build.py`.
- **Linters**:
  - `tools/lint/check_layer_references.py` (0 violations)
  - `tools/lint/check_cmake_dependencies.py` (0 violations)
  - `tools/abi/syscall_abi.py --check` (19 syscalls locked, matches)

---

## 3. Interfaces Between Workstreams

```
+-------------------------------------------------------------+
|                      Theme Package                          |
|             interface/include/bharat/ui/theme.h             |
+------------------------------+------------------------------+
                               |
                               v
+------------------------------+------------------------------+
|                    Boot Event Ring Buffer                   |
|          interface/include/bharat/uapi/boot/boot_events.h   |
|          core/kernel/src/boot/boot_events.c                 |
+--------------+-------------------------------+--------------+
               |                               |
               v                               v
+--------------+---------------+ +---------------+--------------+
|       TinyUI Renderer         | |       LVGL Splash & Diag     |
|   (Early framebuffer /       | |   (Rich GUI / Mobile /       |
|    Compact profile)          | |    Desktop profile)          |
| core/stacks/ui/lcd/tiny_ui.c | | experience/shell/shell_ui.c  |
+------------------------------+ +------------------------------+
               |                               |
               +---------------+---------------+
                               |
                               v
+------------------------------+------------------------------+
|                    Device Profile Engine                    |
|      FULL (x86_64, ARM64, RV64) | COMPACT | HEADLESS (ARM32)|
|          docs/boot-experience/PLATFORM_MATRIX.md            |
+-------------------------------------------------------------+
```

---

## 4. Workstream Implementation Sequence

1. **Phase 1 (BOOT-UX-001)**: Unified Boot Status and Event Pipeline (ring buffer, monotonic timestamps, component error codes, snapshot interface).
2. **Phase 2 (BOOT-UX-002)**: Configurable Bharat-OS Splash Screen & OEM Theme (branding tokens, colors, asset configuration, graceful fallback).
3. **Phase 3 (BOOT-UX-003)**: Graphical Kernel Boot Diagnostics Window (live event view, timestamp/status display, navigation, truncation).
4. **Phase 4 (PLATFORM-UX-004)**: Architecture & Device Profiles (`PLATFORM_MATRIX.md`, target definitions).
5. **Phase 5 (DEMO-UX-005)**: Cross-Architecture QEMU Demo Runner (`tools/run_qemu_matrix.py` enhanced verdicts and reporting).
6. **Phase 6 & 7**: Multi-target integration, host tests, and QEMU smoke matrix.
