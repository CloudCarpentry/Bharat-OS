---
title: Target YAML Authoring Guide
status: Draft
owner: Documentation Working Group
last_updated: 2026-04-25
tags:
  - docs
  - dev
see_also:
  - README.md
---
# Target YAML Authoring Guide

This guide explains how to define declarative targets for the Bharat-OS build and execution pipeline. Target YAML files describe the full lifecycle of a target: how it is built, packaged, run, flashed, and debugged.

Targets are stored in `delivery/targets/` (preferred, e.g., `delivery/targets/qemu/`). During migration, legacy `tools/targets/qemu/` path references are accepted via compatibility aliasing.

## YAML Schema Overview

A target specification contains several top-level sections:

- **Metadata**: Identification (`name`, `kind`, `arch`, `board`).
- **Profiles**: Feature selection (`device_profile`, `personality_profile`, `execution_profile`).
- **build**: CMake compilation instructions.
- **kernel**: Output mapping for canonical artifacts.
- **boot**: Boot protocol and handoff expectations.
- **package**: Instructions for deriving target-specific binaries from canonical artifacts.
- **run**: Emulator configuration (e.g., QEMU).
- **flash**: Hardware deployment configuration.
- **debug**: Debugger attachment parameters.

## Minimal QEMU Example

Here is a typical target for a headless ARM64 QEMU execution using direct ELF loading:

```yaml
name: arm64_desktop_headless
kind: qemu_target

arch: arm64
board: qemu-virt-arm64
device_profile: desktop
personality_profile: native
execution_profile: gp

build:
  cmake_preset: arm64-edge
  cmake_defs:
    BHARAT_ARCH_FAMILY: ARM64
    BHARAT_DEVICE_PROFILE: DESKTOP
    BHARAT_PERSONALITY_PROFILE: NATIVE
    BHARAT_TARGET_BOARD: virt
    BHARAT_BOOT_GUI: OFF

kernel:
  canonical_artifacts:
    elf: core/kernel/kernel.elf
    map: core/kernel/kernel.map

boot:
  protocol: elf_direct
  artifact_format: elf
  dtb:
    mode: qemu_generated
    required: true
    handoff_register: x0

package:
  transforms: []

run:
  backend: qemu
  machine: virt
  cpu: cortex-a72
  memory: 1024M
  nographic: true
  serial:
    - stdio
  boot_artifact: core/kernel/kernel.elf

debug:
  backend: gdb_remote
  stop_on_entry: false
  gdb_port: 1234
```

## Board Target Example

A physical board will specify flash operations rather than run operations:

```yaml
name: stm32f4_discovery
kind: board_target

arch: arm32
board: stm32f407g-disc1
device_profile: iot
personality_profile: native
execution_profile: rt

build:
  cmake_preset: arm32-embedded
  cmake_defs:
    BHARAT_TARGET_BOARD: stm32f4
    # ...

kernel:
  canonical_artifacts:
    elf: core/kernel/kernel.elf

boot:
  protocol: raw_entry
  artifact_format: raw_bin
  dtb:
    mode: appended

package:
  transforms:
    - type: elf_to_bin
      input: core/kernel/kernel.elf
      output: flash_payload.bin

flash:
  backend: openocd
  artifact: flash_payload.bin
  probe: stlink-v2-1
  verify: true
  reset_after_flash: true

debug:
  backend: gdb_remote
  symbols: core/kernel/kernel.elf
```

## Validation Rules

When resolving the YAML, the pipeline performs structural validation (via JSON Schema) and semantic validation.

- A target with a `qemu_target` kind must define a `run` block.
- A `board_target` must define a `flash` block.
- **Protocol-Artifact Safety**: 
  - `elf_direct` protocol requires `artifact_format: elf`.
  - `linux_arm64` requires a real Linux Image (do not use generic `elf_to_bin` for this).
- If the `boot` block requires an `artifact_format: raw_bin`, but the `kernel.canonical_artifacts.elf` is mapped, a valid `elf_to_bin` package transform must exist.
- If `boot.dtb.required` is true, a `dtb.mode` must be declared.

## Common Mistakes

- **Omitting `boot_artifact`**: The `run` or `flash` block needs to know exactly which file to execute from the packaged output. If you applied an `elf_to_bin` transform, your `boot_artifact` should be the resulting `.bin` file, not the original `.elf`.
- **Mixing Boot Protocol and Artifact Format**: `protocol` defines *how* the handoff happens (e.g., what registers are populated), while `artifact_format` defines *what* is loaded into memory.
- **Using `linux_arm64` for QEMU ELF simulation**: Use `elf_direct` instead. `linux_arm64` assumes a Linux Image binary with a valid 64-byte header.
- **Forgetting PyYAML/jsonschema**: Since the pipeline relies on these for resolution, ensure your development environment has them installed.


## Personality profile values

`personality_profile` and `BHARAT_PERSONALITY_PROFILE` now support `native`, `linux`, and `android` for production headless tri-ISA target definitions.
