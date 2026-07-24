---
title: Kernel Subcomponents Architecture
status: Proposed
owner: Documentation Working Group
last_updated: 2026-04-25
tags:
  - docs
  - architecture
see_also:
  - README.md
---

### Boot Subsystem Refactor
The core boot flow has been heavily refactored:
1. Architecture-agnostic `main.c`: No FDT or Multiboot specific tags are parsed here.
2. `boot_info_t` normalizes all boot info (Multiboot, FDT, UEFI, etc.) in a stable contract payload.
3. Early Boot Layers: `core/kernel/src/core/arch/<arch>/platform` isolates platform-specific assumptions (like QEMU UART locations) from generic boot paths.
4. Framebuffer Fixes: Identity mapping has been dropped. Video metadata is structurally checked against bounds (no overflow, no overlap with core/kernel/reserved/module memory), and mapped explicitly after VMM initialization (`boot_video_map`).

### Note on Cross-Architecture Compilation
While the structure for `arm64`, `riscv64`, `arm32`, and `riscv32` are in place (through respective `entry.c` and platform fallbacks), their cross-build validation remains strictly **pending** until necessary cross-compiler toolchains are configured in the CI/CD environment. The `x86_64` architecture has been completely validated and confirmed to support this modular flow.
