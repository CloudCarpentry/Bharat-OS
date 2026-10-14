---
title: Google/Jules-Style Agent Notes
status: Active
owner: Documentation Working Group
last_updated: 2026-10-14
tags:
  - docs
  - ai-agents
  - platforms
see_also:
  - README.md
  - AGENTS.md
---
# Google/Jules-Style Agent Policy & Verification Guide

Google Jules automatically ingests root [AGENTS.md](../../AGENTS.md) when entering the repository.
To minimize context overhead and maximize architectural correctness, Jules follows the 4-pillar constitution and uses canonical skills in `.agents/skills/`.

## Primary Directives for Google Jules

1. **Context-Pack First**:
   Before planning or writing code, Jules runs:
   ```bash
   python3 tools/agents/context_pack.py --area <area-name> --format compact
   ```
   This gives Jules the exact contracts, sources, skills, and constraints in 40 lines.

2. **Mandatory QEMU Installation**:
   Ensure all 5 target hardware QEMU emulators are installed:
   - `qemu-system-x86_64` (x86_64)
   - `qemu-system-aarch64` (arm64)
   - `qemu-system-riscv64` (riscv64)
   - `qemu-system-arm` (arm32)
   - `qemu-system-riscv32` (riscv32)

   *Package setup command (Debian/Ubuntu):*
   ```bash
   sudo apt-get update && sudo apt-get install -y qemu-system-x86 qemu-system-arm qemu-system-misc
   ```

3. **Mandatory Build & Execution Matrix**:
   Jules must verify boot success across all 5 canonical architecture targets:

   ```bash
   # Automated all-in-one verification
   python3 tools/agents/verify.py --changed-files <changed_files>
   python3 tools/run_qemu_matrix.py --headless --smoke --all-arch
   ```

   Individual target smoke commands:
   ```bash
   # Linux / macOS / WSL
   ./tools/build.sh all --target-yaml delivery/targets/qemu/x86_64_desktop_headless.yaml --smoke
   ./tools/build.sh all --target-yaml delivery/targets/qemu/arm64_desktop_headless.yaml --smoke
   ./tools/build.sh all --target-yaml delivery/targets/qemu/riscv64_desktop_headless.yaml --smoke
   ./tools/build.sh all --target-yaml delivery/targets/qemu/arm32_mmu_lite_headless.yaml --smoke
   ./tools/build.sh all --target-yaml delivery/targets/qemu/riscv32_mmu_lite_headless.yaml --smoke

   # Windows PowerShell
   .\tools\build.ps1 all --target-yaml delivery/targets/qemu/x86_64_desktop_headless.yaml --smoke
   .\tools\build.ps1 all --target-yaml delivery/targets/qemu/arm64_desktop_headless.yaml --smoke
   .\tools\build.ps1 all --target-yaml delivery/targets/qemu/riscv64_desktop_headless.yaml --smoke
   .\tools\build.ps1 all --target-yaml delivery/targets/qemu/arm32_mmu_lite_headless.yaml --smoke
   .\tools\build.ps1 all --target-yaml delivery/targets/qemu/riscv32_mmu_lite_headless.yaml --smoke
   ```

4. **No Unverified Completion**:
   Never mark a task `PASS` without empirical runtime evidence (`[Run] PASS`). If any emulator or toolchain is missing, mark as `BLOCKED` until installed.
