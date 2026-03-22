# Host Build/Test/Run Guide (Linux + Windows)

This document provides a practical, cross-host workflow to **build**, **test**, and **run** Bharat-OS kernels on:

- Linux hosts (native or WSL)
- Windows hosts (PowerShell)

It is designed for teams working across multiple architectures and simulators/emulators.

---

## 1) Scope and Goals

Use this guide when you need to:

- Build kernels for `riscv64`, `arm64`, or `x86_64`
- Run host-side tests in CI/local dev
- Boot kernels in simulators (QEMU first, then architecture-specific tools)
- Keep Linux and Windows command flows consistent

---

## 2) Recommended Simulator Selection

| Goal | Primary Tool | Secondary Tool |
|---|---|---|
| Fastest bring-up | QEMU | TinyEMU |
| RISC-V ISA fidelity | Spike | Dromajo |
| Embedded/IoT peripheral simulation | Renode | QEMU board models |
| Micro-architecture experiments | gem5 | FireSim |
| x86 boot/debug | Bochs | QEMU |

> For Bharat-OS day-to-day development, start with **QEMU + host tests**.

---

## 3) Host Setup Baseline

Follow the repository environment setup in `docs/ENV_PREP.md` first.

Minimum expected tools on both hosts:

- CMake (preset support)
- Ninja
- Clang/LLVM (or GCC where documented)
- QEMU binaries for target architectures
- Python (for some scripts/tooling)

---

## 4) Linux Host Workflow

## 4.1 Build

```bash
# Example: RISC-V64
chmod +x tools/build.sh
./tools/build.sh riscv64

# Example: ARM64
./tools/build.sh arm64

# Example: clean rebuild
./tools/build.sh riscv64 --clean
```

## 4.2 Run host tests

```bash
cmake --preset tests-host
cmake --build --preset build-tests
ctest --preset run-tests
```

## 4.3 Run in QEMU

```bash
# RISC-V64
./tools/build.sh riscv64 --run

# ARM64
./tools/build.sh arm64 --run

# x86_64
./tools/build.sh x86_64 --run
```

For manual QEMU invocation, use generated kernel artifacts from the corresponding build directory.

---

## 5) Windows Host Workflow (PowerShell)

## 5.1 Build

```powershell
# RISC-V64
.\tools\build.ps1 -Arch riscv64

# ARM64
.\tools\build.ps1 -Arch arm64

# x86_64
.\tools\build.ps1 -Arch x86_64

# clean rebuild
.\tools\build.ps1 -Arch riscv64 -Clean
```

## 5.2 Run host tests

```powershell
cmake --preset tests-host
cmake --build --preset build-tests
ctest --preset run-tests
```

## 5.3 Run in QEMU

```powershell
.\tools\build.ps1 -Arch riscv64 -Run
.\tools\build.ps1 -Arch arm64 -Run
.\tools\build.ps1 -Arch x86_64 -Run
```

---

## 6) CMake Preset-First Flow (Host Agnostic)

If your team prefers explicit preset usage:

```bash
# Configure + build kernel (sample)
cmake --preset riscv64-elf-debug
cmake --build --preset build-riscv64-kernel

# Run host tests
cmake --preset tests-host
cmake --build --preset build-tests
ctest --preset run-tests
```

Use this flow in CI to keep behavior consistent across Linux and Windows runners.

---

## 7) Multi-Simulator Validation (Optional Advanced)

After QEMU pass, run architecture-specific deep checks:

- **Spike** for RISC-V ISA behavior (`rv32/rv64` variants)
- **Renode** for embedded peripheral interaction and multi-node scenarios
- **gem5** for cache/memory/timing studies
- **Bochs** for x86 early boot debugging
- **Dromajo** for RISC-V RTL co-sim divergence checks

Suggested progression:

1. Build + host tests pass
2. Boot smoke test in QEMU
3. ISA/peripheral/micro-arch focused simulator passes
4. Promote artifact to integration test stage

---

## 8) CI-Friendly Command Blocks

## Linux CI stage

```bash
cmake --preset tests-host
cmake --build --preset build-tests
ctest --preset run-tests
./tools/build.sh riscv64
./tools/build.sh arm64
./tools/build.sh x86_64
```

## Windows CI stage

```powershell
cmake --preset tests-host
cmake --build --preset build-tests
ctest --preset run-tests
.\tools\build.ps1 -Arch riscv64
.\tools\build.ps1 -Arch arm64
.\tools\build.ps1 -Arch x86_64
```

---

## 9) Troubleshooting Checklist

- Ensure toolchain binaries are on `PATH`.
- Verify preset names using `cmake --list-presets`.
- Confirm QEMU binary exists for selected architecture.
- On Windows, run PowerShell with script execution enabled for local scripts.
- If runtime fails but build passes, capture serial output and compare with known-good QEMU boot logs.

---

## 10) Suggested Team Policy

- Gate merges on host tests + at least one target build per architecture tier.
- Use QEMU smoke boot as required for kernel-touching changes.
- Reserve gem5/FireSim/Renode deep scenarios for nightly or profile-specific pipelines.

