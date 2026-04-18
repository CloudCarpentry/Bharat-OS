# Build, Packaging, Run, Flash, and Debug Contract

This document defines the architecture and rules for the Bharat-OS delivery pipeline.
The goal is to maintain a strict separation of concerns between compilation (CMake), boot logic (boot/), artifact transformations (packaging), and execution/deployment (run/flash).

## Pipeline Stages

1. **Configure & Build (CMake)**
   - Responsibility: Produce the canonical `kernel.elf`.
   - Input: Source code, CMake definitions.
   - Output: `kernel.elf`, optional `.map` files, `compile_commands.json`.
   - **Rule**: CMake does *not* know how to package a raw binary, construct a flash image, or run QEMU. It strictly produces the compiled output.

2. **Package (tools/package/)**
   - Responsibility: Transform the canonical ELF into derived artifacts suitable for specific boot protocols or hardware.
   - Input: `kernel.elf`, target YAML packaging rules.
   - Output: Derived artifacts (e.g., `kernel.bin`, `flash.bin`, bundled DTBs), `run-manifest.json`, `flash-manifest.json`.
   - **Rule**: Packaging transforms must be idempotent and defined explicitly in the target spec.

3. **Run (tools/run/)**
   - Responsibility: Launch the target in an emulator (e.g., QEMU, Renode).
   - Input: `run-manifest.json`
   - **Rule**: Emulator wrappers strictly consume the manifest. They do not parse ELF files or apply ad-hoc architecture logic.

4. **Flash (tools/flash/)**
   - Responsibility: Deploy the packaged artifact to physical hardware.
   - Input: `flash-manifest.json`

5. **Debug (tools/debug/)**
   - Responsibility: Attach a debugger (GDB, LLDB) to the running emulator or physical board.
   - Input: `run-manifest.json` or `flash-manifest.json` (for symbol path mapping).

## Artifact Taxonomy

- **Canonical Artifact**: `kernel.elf`. The untampered compiler output.
- **Derived Artifact**: Transformed binaries (e.g., `raw_bin`, `boot_image`).
- **Manifests**: JSON descriptions of what to execute or flash.

## DTB Ownership Model
DTB (Device Tree Blob) logic depends on the boot contract:
- **QEMU Generated**: Emulator provides the DTB at runtime. Packaging ignores DTB.
- **Bundled**: Packager injects DTB into the payload or places it adjacent to the binary.

## Target YAML Specifications
Targets are declaratively described in YAML files (`tools/targets/`). The schema validation fails early if incompatible configs are present (e.g., `linux_arm64` protocol with `artifact_format: elf`).
