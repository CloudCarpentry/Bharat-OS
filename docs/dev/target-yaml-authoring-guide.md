# Target YAML Authoring Guide

Bharat-OS relies on declarative YAML files to define build targets, covering everything from CMake presets to packaging, running, and flashing. This replaces scattered legacy CLI flags and logic inside the build scripts.

## Location
Targets belong in:
- `tools/targets/qemu/` for emulator targets.
- `tools/targets/boards/` for physical hardware targets.

## Structure Overview

A target YAML must contain the following keys:
- `name`: Unique identifier for the target.
- `arch`: Target architecture (e.g., `arm64`, `x86_64`).
- `board`: Machine or board identifier (e.g., `qemu-virt-arm64`, `shakti-c`).
- `build`: CMake configuration properties (`cmake_preset`, `cmake_defs`).
- `kernel`: Path definitions for the core build artifact (`kernel.elf`).
- `boot`: Boot protocol and artifact requirements.
- `package`: Required transformations (e.g., `elf_to_bin`).
- `run` (Optional): Instructions for running in an emulator.
- `flash` (Optional): Instructions for deploying to hardware.

## Example: ARM64 QEMU Headless

```yaml
name: arm64_desktop_headless_qemu
arch: arm64
board: virt

build:
  cmake_preset: arm64-edge
  cmake_defs:
    BHARAT_ARCH_FAMILY: ARM64
    BHARAT_DEVICE_PROFILE: DESKTOP
    BHARAT_PERSONALITY_PROFILE: LINUX
    BHARAT_TARGET_BOARD: virt
    BHARAT_BOOT_GUI: OFF

kernel:
  canonical_artifact: build/arm64_desktop_headless_qemu/kernel/kernel.elf
  entry_contract: kernel_main

boot:
  protocol: linux_arm64
  artifact_format: raw_image
  dtb:
    mode: qemu_generated
    required_at_entry: true
    handoff_register: x0

package:
  transforms:
    - type: elf_to_bin
      input: build/arm64_desktop_headless_qemu/kernel/kernel.elf
      output: build/arm64_desktop_headless_qemu/packaged/kernel.bin

run:
  method: qemu
  machine: virt
  cpu: cortex-a72
  memory: 1024M
  serial:
    - stdio
  display: none
  boot_artifact: build/arm64_desktop_headless_qemu/packaged/kernel.bin
  dtb:
    mode: qemu_generated
  extra_args:
    - "-nographic"
```

## Validation
Always validate new targets against the JSON schema defined in `tools/schemas/target.schema.yaml`.
