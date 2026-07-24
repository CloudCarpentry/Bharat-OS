---
title: Local Multi-Architecture Verification Guide
status: Draft
owner: Documentation Working Group
last_updated: 2026-04-25
tags:
  - docs
  - dev
see_also:
  - README.md
---
# Local Multi-Architecture Verification Guide

This guide describes how to verify Bharat-OS across all supported architectures and profiles using local tools.

## 1. Native Verification (Recommended for Windows/Native)

If you don't have Docker running (required for `act`), you should use the native Python build wrapper. This uses your local toolchain (Clang, LLVM, QEMU) as defined in [ENV_PREP.md](../ENV_PREP.md).

### Basic Command Pattern
```bash
python3 tools/build.py <build_name> --clean --configure --build --run-tests
```

### Common Combinations
| Target | Command | Status |
| :--- | :--- | :--- |
| **x86_64 Desktop** | `python tools/build.py default_dev --run-tests` | ✅ Verified |
| **ARM64 Desktop** | `python tools/build.py arm64_desktop_mmu --run-tests` | ✅ Verified |
| **ARM32 Edge** | `python tools/build.py arm32_virt_mmu --run-tests` | ⚠️ Stabilizing |
| **RISCV64 Desktop** | `python tools/build.py riscv64_desktop_mmu --run-tests` | ✅ Verified |
| **RISCV32 Robot** | `python tools/build.py riscv32_robot_debug --run-tests` | ⚠️ Stabilizing |

### Architecture-Specific Boot Notes

*   **x86_64**: QEMU requires a 32-bit ELF for multiboot compatibility. The build script automatically uses `llvm-objcopy` to produce `kernel32.elf` and boots it.
*   **ARM64**: Requires passing `-machine virt -cpu cortex-a57` to QEMU. The kernel also expects a valid Device Tree Blob (FDT) pointer passed in `x0`, which is correctly preserved during the early boot assembly phase. It boots from a flat binary format (`Image`).
*   **ARM32**: Targets the ARMv7-A ISA. Uses `-machine virt -cpu cortex-a15`. Linked with `lld` to avoid legacy GCC dependency.
*   **RISCV64**: Requires the `-march=rv64gc` ISA extensions for the compiler to prevent floating-point and compressed instruction errors. It uses `-machine virt` for the QEMU machine type.
*   **RISCV32**: Targets the generic RV32G ISA. Uses `-machine virt`. Required specialized `hal_pt_riscv32.c` and 32/64-bit agnostic assembly macros.

---

## 2. Local CI Verification (Using `act`)

If you have Docker Desktop (Windows) or Docker Engine (Linux) running, you can use `act` to run the exact GitHub Actions environment locally.

### Prerequisites
1.  **Docker**: Ensure Docker is running. On Windows, check if the Docker Desktop icon is in the tray.
2.  **Act**: Install via `winget install nektos.act` (Windows) or `brew install act` (macOS/Linux).

### Execution Examples
To run the full matrix sequentially (All 8+ combinations):
```bash
act -W .github/workflows/e2e-qemu.yml
```

To run only a specific build name to save time:
```bash
act -W .github/workflows/e2e-qemu.yml --matrix build_name:x86_64_desktop_mmu
```

### Troubleshooting "Docker Connection Failed"
If you see `level=warning msg="Couldn't get a valid docker connection"`, ensure:
-   Docker Desktop is actually started.
-   On Windows, ensure "Expose daemon on tcp://localhost:2375 without TLS" is NOT required, or if using WSL, that the WSL integration is enabled.
-   Try running the terminal as **Administrator**.

---

## 3. Reference: Full Build Matrix
- `default_dev` (x86_64)
- `arm64_desktop_mmu`
- `arm32_virt_mmu`
- `riscv64_desktop_mmu`
- `riscv32_robot_debug`
- `riscv32_edge_mmu_lite`

---

## 4. Execution Strategy & Validation

For robust multi-arch verification, follow this sequence:

1. **Clean & Configure**: Always use `--clean --configure` when switching between architectures to ensure the CMake cache does not contain stale host or targeting paths.
2. **Sequential Build**: Use the naming matrix provided in Phase 2 above.
3. **Log Analysis**: Search for the `Bharat-OS` banner in the serial output.
4. **Failure Recovery**: On 32-bit targets, monitor for the specialized "Runtime Tier" logs to ensure the correct architecture-agnostic assembly macros are active.
