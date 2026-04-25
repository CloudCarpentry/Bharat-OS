---
title: Migrating from build_config.json
status: Draft
owner: Documentation Working Group
last_updated: 2026-04-25
tags:
  - docs
  - dev
see_also:
  - README.md
---
# Migrating from build_config.json

This document explains the transition from the legacy `build_config.json` to the new declarative Target YAML pipeline.

## Why the transition?

The original `build_config.json` combined target profiles but left much of the packaging, QEMU construction, and deployment logic hardcoded into Python and Shell wrappers. The new YAML-driven approach separates concerns by defining declarative schemas for the build, package, run, flash, and debug pipelines.

The old system was characterized by implied configurations, where scripts guessed how to execute an ELF file based on architecture strings. The new pipeline requires build/runtime truth to be explicit and enforceable.

## Migration Status

- **Current State**: Hybrid. We are actively migrating a representative subset of targets to YAML (`tools/targets/`). `tools/build.py` natively supports the YAML pipeline while maintaining a compatibility shim that normalizes `build_config.json` targets into the new internal model.
- **Legacy Config**: `build_config.json` remains functional but is considered **deprecated**. No new features, runner overrides, or board support will be added to the legacy path.

## How to Migrate a Target

### 1. Locate your target in `build_config.json`

Example legacy entry:
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

### 2. Create the YAML Spec

Create a YAML file in `tools/targets/qemu/` or `tools/targets/boards/`. For example: `tools/targets/qemu/riscv64_desktop_headless.yaml`.

### 3. Mapping Table

Map the old config fields into the new declarative YAML sections:

| Legacy `build_config.json` | New Target YAML |
| :--- | :--- |
| Config key (`riscv64_...`) | `name` (top-level) |
| `arch` | `arch` |
| `board` | `board` |
| `profile` | `device_profile` |
| `personality` | `personality_profile` |
| `preset` | `build.cmake_preset` |
| Implicit CMake defines | Explicit `build.cmake_defs` |
| Hardcoded kernel output path | Explicit `kernel.canonical_artifacts.elf` |
| Implicit boot handoff logic | Explicit `boot.protocol` and `boot.dtb` strategy |
| Script-embedded `objcopy` logic | Explicit `package.transforms` (e.g., `elf_to_bin`) |
| Hardcoded QEMU flags | Explicit `run` block properties (`machine`, `memory`, `serial`, etc.) |

**Important Addition:** You must explicitly define the boot protocol (e.g., `linux_arm64`, `multiboot2`, `opensbi_payload`). If your runner requires a `.bin` instead of an `.elf`, you must define an `elf_to_bin` package transform and set the `run.boot_artifact` to the resulting `.bin`.

### 4. Test the Target

Test the target using the new explicit subcommands and target flags:

```bash
# Build the target
python tools/build.py build --target-yaml tools/targets/qemu/riscv64_desktop_headless.yaml

# Run the target
python tools/build.py run --target-yaml tools/targets/qemu/riscv64_desktop_headless.yaml

# Or do both
python tools/build.py all --target-yaml tools/targets/qemu/riscv64_desktop_headless.yaml
```

### 5. Update CI

Once proven locally, ensure the YAML file is referenced correctly in CI matrix configurations. Do not delete the entry in `build_config.json` until Phase 2 cleanup is complete across the repo.
