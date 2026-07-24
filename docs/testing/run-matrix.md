---
title: Unified Multi-Arch SMP Run Matrix
status: Draft
owner: Documentation Working Group
last_updated: 2026-04-25
tags:
  - docs
  - testing
see_also:
  - README.md
---
# Unified Multi-Arch SMP Run Matrix

Bharat-OS supports a systematic execution matrix to test Single-core (UP) and Multi-core (SMP) operations across varying architectures and execution profiles.

## Architecture & Board Support

The runtime behavior and SMP capability differ based on the target architecture and the underlying machine/board selected:

| Arch    | Board           | Runner                | SMP Supported | Notes                                      |
| ------- | --------------- | --------------------- | ------------- | ------------------------------------------ |
| x86_64  | qemu-virt       | `qemu-system-x86_64`  | ✅           | Stable SMP. Uses `q35` machine.            |
| arm64   | virt            | `qemu-system-aarch64` | ✅           | Stable SMP. Uses `virt` machine.           |
| riscv64 | virt            | `qemu-system-riscv64` | ✅           | Stable SMP. Uses `virt` machine.           |
| arm32   | virt            | `qemu-system-arm`     | ✅           | Cortex-A15 SMP via QEMU virt.              |
| arm32   | avh-corstone310 | `VHT_Corstone_SSE-310`| ❌           | Cortex-M single-core board. No SMP support.|
| riscv32 | virt            | `qemu-system-riscv32` | ⚠️           | Experimental.                              |

## Usage

The centralized runner is `tools/build.py` (which powers the wrappers `./build.sh` and `.\build.ps1`).

### Specify CPUs

To test an SMP configuration, you can pass the `--cpus` option alongside the run flags. Note that SMP must be supported by the run target as defined in the matrix above.

```bash
# Run on 1 CPU
./build.sh run --target x86_64_desktop_mmu

# Run on 4 CPUs
./build.sh run --target x86_64_desktop_mmu

# Run ARM32 with SMP (virt path)
./build.sh run --target arm32_virt_mmu
```

If you specify `--cpus N` (where N > 1) for a board without SMP support (like `avh-corstone310`), the runner will intentionally fail rather than pretending to provide SMP capabilities.

### Matrix Mode

For a powerful, automated approach, you can run a pre-curated test matrix using the `--matrix` option:

```bash
./build.sh all --matrix
```

This sequentially configures, builds, and runs headless tests against a curated list of build configurations and CPU counts, including single-core and multi-core configurations across architectures. At the conclusion of the matrix execution, a summary is provided showing the pass/fail status of each configuration.
