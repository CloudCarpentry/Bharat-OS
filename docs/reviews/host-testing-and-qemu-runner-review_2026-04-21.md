---
title: Host Testing & QEMU Runner Review (2026-04-21)
status: Draft
owner: Documentation Working Group
last_updated: 2026-04-25
tags:
  - docs
  - reviews
see_also:
  - README.md
---
# Host Testing & QEMU Runner Review (2026-04-21)

## Scope
This review replaces the retired `docs/current/host_master.md` document with a code-backed snapshot of the current implementation.

## What changed since the old host document

1. **Host stubs are no longer centralized in a single file only.**
   - `quality/tests/host/host_stubs.c` still exists, but host tests now also include local test stubs and architecture-specific harness wiring.
2. **Run pipeline is manifest-driven.**
   - QEMU execution comes from `tools/build.py` + packaging manifests and then `tools/run/runner_qemu.py`, not from host-test docs.
3. **Headless QEMU support is target-driven.**
   - QEMU target YAMLs such as `tools/targets/qemu/x86_64_desktop_headless.yaml` define `nographic`, serial routing, and machine profile.

## Code task identified and implemented

### Task
Fix x86_64 build break caused by unresolved `corecore/halcore/hal_cpu_topology.h` includes while compiling core/kernel/HAL and subsystem sources.

### Root cause
The topology header existed only under `corecore/hal/include/corecore/hal/`, while several core/kernel/subsystem sources included it as `corecore/halcore/hal_cpu_topology.h` through include paths rooted at `core/kernel/include` and service include trees.

### Fix
- Added `${CMAKE_SOURCE_DIR}/corecore/hal/include` to `hal_common` private include directories.
- Added a canonical exported copy at `core/kernel/include/corecore/halcore/hal_cpu_topology.h` so core/kernel/subsystem include roots resolve consistently without changing header precedence for other HAL headers.

## Verification done

- Installed QEMU system runners (x86, arm/aarch64, riscv) in the environment.
- Installed Python YAML dependencies required by `tools/build.py` target resolver.
- Rebuilt x86_64 headless QEMU target and executed headless run workflow.
