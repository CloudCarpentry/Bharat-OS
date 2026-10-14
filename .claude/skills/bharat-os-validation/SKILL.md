---
name: bharat-os-validation
description: Apply before completing any Bharat-OS code, configuration, build, architecture, ABI, or documentation task that requires executable evidence.
---

# Bharat-OS Validation Skill

## Evidence Discipline

1. **Empirical Evidence Only**: A command or test gate is `PASS` ONLY if it was executed and returned the expected output evidence.
2. **Missing Dependencies are BLOCKED**: If an emulator, toolchain, cross-compiler, YAML target, or test harness is missing or unavailable, mark the gate as `BLOCKED: <reason>`. **Never report a missing or skipped test as PASS.**
3. **No Target Substitution**: Never silently substitute another target for one of the mandatory canonical targets.

## Mandatory Five-Target Architecture Matrix

Bharat-OS formally qualifies across five canonical hardware architectures:

| Architecture | Target Profile YAML | Protection Model | Emulator Required |
|---|---|---|---|
| **x86_64** | `delivery/targets/qemu/x86_64_desktop_headless.yaml` | MMU_FULL | `qemu-system-x86_64` |
| **ARM64** | `delivery/targets/qemu/arm64_desktop_headless.yaml` | MMU_FULL | `qemu-system-aarch64` |
| **RISC-V 64** | `delivery/targets/qemu/riscv64_desktop_headless.yaml` | MMU_FULL | `qemu-system-riscv64` |
| **ARM32** | `delivery/targets/qemu/arm32_mmu_lite_headless.yaml` | MMU_LITE | `qemu-system-arm` |
| **RISC-V 32** | `delivery/targets/qemu/riscv32_mmu_lite_headless.yaml` | MMU_LITE | `qemu-system-riscv32` |

## Validation Sequence

### Step 1: Automated Agent Verifier
Run the integrated agent verifier to check affected files, linters, and focused tests:
```bash
python3 tools/agents/verify.py --changed-files <file1> <file2>
```

### Step 2: Layer & Architecture Linters
Verify layer boundaries and dependency graph:
```bash
python3 tools/lint/check_layer_references.py
python3 tools/lint/check_cmake_dependencies.py
```

### Step 3: Syscall ABI Verification (Mandatory for ABI/Syscall Changes)
```bash
python3 tools/abi/syscall_abi.py --check
```

### Step 4: Full Multi-Architecture QEMU Matrix Gate
Execute the headless smoke matrix across all 5 architectures:
```bash
python3 tools/run_qemu_matrix.py --headless --smoke --all-arch
```

If validating individual targets manually via build script:
```bash
# On Linux / macOS / WSL:
./tools/build.sh all --target-yaml delivery/targets/qemu/x86_64_desktop_headless.yaml --smoke
./tools/build.sh all --target-yaml delivery/targets/qemu/arm64_desktop_headless.yaml --smoke
./tools/build.sh all --target-yaml delivery/targets/qemu/riscv64_desktop_headless.yaml --smoke
./tools/build.sh all --target-yaml delivery/targets/qemu/arm32_mmu_lite_headless.yaml --smoke
./tools/build.sh all --target-yaml delivery/targets/qemu/riscv32_mmu_lite_headless.yaml --smoke

# On Windows PowerShell:
.\tools\build.ps1 all --target-yaml delivery/targets/qemu/x86_64_desktop_headless.yaml --smoke
.\tools\build.ps1 all --target-yaml delivery/targets/qemu/arm64_desktop_headless.yaml --smoke
.\tools\build.ps1 all --target-yaml delivery/targets/qemu/riscv64_desktop_headless.yaml --smoke
.\tools\build.ps1 all --target-yaml delivery/targets/qemu/arm32_mmu_lite_headless.yaml --smoke
.\tools\build.ps1 all --target-yaml delivery/targets/qemu/riscv32_mmu_lite_headless.yaml --smoke
```

## Completion Report Table

Every completion report must contain this exact verification matrix:

| Gate | Target / Scope | Command | Status | Evidence / Blocker |
|---|---|---|---|---|
| Linters | Layer & CMake | `check_layer_references.py` | PASS/FAIL | Observed zero violations |
| ABI | Syscall manifest | `syscall_abi.py --check` | PASS/SKIPPED | ABI hash matches lock |
| QEMU x86_64 | Desktop Headless | `run_qemu_matrix.py --arch x86_64` | PASS/BLOCKED | Emitted boot marker |
| QEMU ARM64 | Desktop Headless | `run_qemu_matrix.py --arch arm64` | PASS/BLOCKED | Emitted boot marker |
| QEMU RISC-V64 | Desktop Headless | `run_qemu_matrix.py --arch riscv64` | PASS/BLOCKED | Emitted boot marker |
| QEMU ARM32 | MMU-Lite Headless | `run_qemu_matrix.py --arch arm32` | PASS/BLOCKED | Emitted boot marker |
| QEMU RISC-V32 | MMU-Lite Headless | `run_qemu_matrix.py --arch riscv32` | PASS/BLOCKED | Emitted boot marker |
| All-Arch Gate | 5 Architectures | `run_qemu_matrix.py --all-arch` | PASS/BLOCKED | Summary verdict |
