# Local Multi-Architecture Verification Guide

This guide describes how to verify Bharat-OS across all supported architectures and profiles using local tools.

## 1. Native Verification (Recommended for Windows/Native)

If you don't have Docker running (required for `act`), you should use the native Python build wrapper. This uses your local toolchain (Clang, LLVM, QEMU) as defined in [ENV_PREP.md](../ENV_PREP.md).

### Basic Command Pattern
```bash
python3 tools/build.py <build_name> --clean --configure --build --run-tests
```

### Common Combinations
| Target | Command |
| :--- | :--- |
| **x86_64 Desktop** | `python3 tools/build.py x86_64_desktop_mmu --run-tests` |
| **ARM64 Desktop** | `python3 tools/build.py arm64_desktop_mmu --run-tests` |
| **RISCV64 Desktop** | `python3 tools/build.py riscv64_desktop_mmu --run-tests` |
| **Automotive (ARM64)** | `python3 tools/build.py arm64_automobile_debug --run-tests` |
| **Laptop (x86_64)** | `python3 tools/build.py x86_64_laptop_debug --run-tests` |

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
All build names available in `build_config.json`:

- `x86_64_desktop_mmu`
- `arm64_desktop_mmu`
- `riscv64_desktop_mmu`
- `arm64_automobile_debug`
- `riscv64_ev_automobile_debug`
- `x86_64_laptop_debug`
- `arm64_laptop_debug`
- `riscv64_laptop_debug`
- `arm32_edge_mpu`
- `riscv32_edge_mmu_lite`
