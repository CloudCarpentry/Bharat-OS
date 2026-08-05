---
title: Codex Runtime & Agent Policy Guide
status: Active
owner: Documentation Working Group
last_updated: 2026-08-05
tags:
  - docs
  - ai-agents
  - platforms
see_also:
  - README.md
  - AGENTS.md
---
# OpenAI Codex-Style Agent Policy & Verification Guide

## Primary Directives for OpenAI Codex

1. **Mandatory QEMU Installation**:
   Prior to executing builds or tests, ensure all target hardware QEMU emulators are installed:
   - `qemu-system-x86_64` (x86_64)
   - `qemu-system-aarch64` (arm64)
   - `qemu-system-riscv64` (riscv64)
   - `qemu-system-arm` (arm32)
   - `qemu-system-riscv32` (riscv32)

   *Package setup command (Debian/Ubuntu):*
   ```bash
   sudo apt-get update && sudo apt-get install -y qemu-system-x86 qemu-system-arm qemu-system-misc
   ```

2. **Mandatory Build & Execution Matrix**:
   Codex must compile, build, and execute all target configurations (headless smoke and interactive GUI runs) to verify boot success across all 5 supported architectures:

   ```bash
   # WSL / Linux / macOS
   ./tools/build.sh all --target-yaml delivery/targets/qemu/x86_64_desktop_headless.yaml --smoke
   ./tools/build.sh all --target-yaml delivery/targets/qemu/x86_64_desktop_gui.yaml --interactive
   ./tools/build.sh all --target-yaml delivery/targets/qemu/arm64_desktop_headless.yaml --smoke
   ./tools/build.sh all --target-yaml delivery/targets/qemu/arm64_desktop_gui.yaml --interactive
   ./tools/build.sh all --target-yaml delivery/targets/qemu/riscv64_desktop_headless.yaml --smoke
   ./tools/build.sh run --target-yaml delivery/targets/qemu/riscv64_desktop_gui.yaml --interactive
   ./tools/build.sh all --target-yaml delivery/targets/qemu/arm32_mmu_lite_headless.yaml --smoke
   ./tools/build.sh all --target-yaml delivery/targets/qemu/riscv32_mmu_lite_headless.yaml --smoke
   ```

   ```powershell
   # Windows PowerShell
   .\tools\build.ps1 all --target-yaml delivery/targets/qemu/x86_64_desktop_headless.yaml --smoke
   .\tools\build.ps1 all --target-yaml delivery/targets/qemu/x86_64_desktop_gui.yaml --interactive
   .\tools\build.ps1 all --target-yaml delivery/targets/qemu/arm64_desktop_headless.yaml --smoke
   .\tools\build.ps1 all --target-yaml delivery/targets/qemu/arm64_desktop_gui.yaml --interactive
   .\tools\build.ps1 all --target-yaml delivery/targets/qemu/riscv64_desktop_headless.yaml --smoke
   .\tools\build.ps1 run --target-yaml delivery/targets/qemu/riscv64_desktop_gui.yaml --interactive
   .\tools\build.ps1 all --target-yaml delivery/targets/qemu/arm32_mmu_lite_headless.yaml --smoke
   .\tools\build.ps1 all --target-yaml delivery/targets/qemu/riscv32_mmu_lite_headless.yaml --smoke
   ```

3. **No Unverified Completion**:
   Never mark a task `PASS` without empirical runtime evidence (`[Run] PASS`). If any emulator or toolchain is missing, mark as `BLOCKED` until installed.

