# Migrating from build_config.json

This document explains the transition from the legacy `build_config.json` to the new declarative Target YAML pipeline.

## Why the transition?
The original `build_config.json` combined target profiles but left much of the packaging, QEMU construction, and deployment logic hardcoded into Python and Shell wrappers. The new YAML-driven approach separates concerns: defining declarative schemas for build, package, run, flash, and debug pipelines.

## Migration Status
- **Current State**: Hybrid. We are actively migrating a representative subset of targets to YAML (`tools/targets/`). `tools/build.py` natively supports the YAML pipeline, but maintains fallback support for `build_config.json`.
- **Legacy Config**: `build_config.json` remains functional but is considered **deprecated**. No new features or runner overrides will be added to the legacy path.

## How to Migrate a Target

1. **Locate your target** in `build_config.json`.
   ```json
   "riscv64_desktop_headless": {
      "preset": "riscv64-edge",
      "arch": "riscv64",
      "profile": "DESKTOP",
      "personality": "NATIVE",
      "board": "virt",
      "display": { "enabled": false, "class": "none" },
      "run": false
    }
   ```
2. **Create a YAML file** in `tools/targets/qemu/` or `tools/targets/boards/` corresponding to the target. For example, `tools/targets/qemu/riscv64_desktop_headless.yaml`.
3. **Map the properties**:
   - `preset` -> `build.cmake_preset`.
   - Identify the canonical ELF path (usually `build/<target_name>/kernel/kernel.elf`).
   - Define the explicit `boot.protocol` (e.g., `linux_arm64`, `multiboot2`, `opensbi_payload`).
   - Define the `package.transforms` if the runner requires a `.bin` instead of an `.elf`.
   - Fully describe the `run` properties previously hardcoded in `runner_qemu.py`.
4. **Test the target** using the new execution path:
   ```bash
   python tools/build.py tools/targets/qemu/riscv64_desktop_headless.yaml --build --run
   ```
5. **Update CI**: Once proven, ensure the YAML file replaces the legacy configuration name in CI matrices. Do not delete the entry in `build_config.json` until Phase 2 cleanup is complete across the repo.
