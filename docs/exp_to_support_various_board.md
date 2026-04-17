# Bharat-OS: Experimental Support Strategy for Various Boards

## Purpose

This document defines a **capability-first hardware support model** for Bharat-OS, so platform support is explicit and technically honest.

Instead of claiming broad board support, Bharat-OS should classify boards by what they can actually run:

- hardware capability → runtime capability → Bharat-OS support level
- no “illusion of support” for MPU-only systems

---

## Unified Hardware Capability Table (2026)

| Tier | Arch     | Board              | RAM    | Storage      | CPU            | Cores | MMU/MPU  | ISA Extensions | Perf Class | Bharat-OS Stage | Can Run Full OS? |
| ---- | -------- | ------------------ | ------ | ------------ | -------------- | ----- | -------- | -------------- | ---------- | --------------- | ---------------- |
| 0    | RISC-V32 | CH32V003           | 2 KB   | 16 KB Flash  | QingKe V2A     | 1     | MPU      | RV32EC         | Ultra-Low  | Kernel bring-up | ❌                |
| 0    | ARM32    | STM32F4 Black Pill | 128 KB | 512 KB Flash | Cortex-M4      | 1     | MPU      | ARMv7E-M       | Ultra-Low  | Kernel bring-up | ❌                |
| 0    | RISC-V32 | GD32VF103          | 32 KB  | Flash        | RV32IMAC       | 1     | MPU      | IMAC           | Ultra-Low  | Kernel bring-up | ❌                |
| 1    | RISC-V32 | ESP32-C6           | 512 KB | SPI Flash    | RV32IMAC       | 1     | MPU      | WiFi6 + IMAC   | Low        | RTOS + kernel   | ❌                |
| 1    | ARM32    | STM32MP1           | 256 MB | eMMC         | Cortex-A7 + M4 | 2     | Hybrid   | ARMv7-A        | Low        | Early OS        | ⚠️ Partial       |
| 2    | RISC-V64 | Milk-V Duo         | 64 MB  | SD           | C906           | 1     | MMU-lite | RV64GCV (0.7)  | Low        | Minimal OS      | ⚠️               |
| 2    | RISC-V64 | Ox64               | 64 MB  | SD/QSPI      | C906           | 1     | MMU-lite | RV64GC         | Low        | Minimal OS      | ⚠️               |
| 2    | ARM64    | Milk-V Duo S       | 512 MB | eMMC         | Cortex-A53     | 1     | MMU      | ARMv8-A        | Low        | Full kernel     | ✅                |
| 2    | ARM64    | Raspberry Pi 3     | 1 GB   | SD           | Cortex-A53     | 4     | MMU      | ARMv8-A        | Low        | Full kernel     | ✅                |
| 3    | ARM32    | BeagleBone Black   | 512 MB | eMMC         | Cortex-A8      | 1     | MMU      | ARMv7-A        | Medium     | Full OS         | ✅                |
| 3    | RISC-V64 | VisionFive 2       | 8 GB   | SD/eMMC/NVMe | JH7110         | 4     | MMU      | RV64GC         | Medium     | Full OS         | ✅                |
| 3    | ARM64    | Raspberry Pi 4     | 8 GB   | SD/USB       | Cortex-A72     | 4     | MMU      | ARMv8.2        | Medium     | Full OS         | ✅                |
| 3    | ARM64    | Raspberry Pi 5     | 16 GB  | NVMe         | Cortex-A76     | 4     | MMU      | ARMv8.2        | High       | Full OS         | ✅                |
| 3    | ARM64    | Rock Pi 5          | 32 GB  | NVMe         | RK3588         | 8     | MMU      | ARMv8.2        | High       | Full OS         | ✅                |
| 4    | ARM64    | Jetson Orin Nano   | 8 GB   | NVMe         | Cortex-A78AE   | 6     | MMU      | CUDA + NEON    | AI         | Advanced OS     | ✅                |
| 4    | ARM64    | Jetson AGX Orin    | 64 GB  | NVMe         | Cortex-A78AE   | 12    | MMU      | GPU + Tensor   | AI         | Advanced OS     | ✅                |
| 4    | RISC-V64 | HiFive Unmatched   | 16 GB  | NVMe         | U74            | 4     | MMU      | RV64GC         | High       | Full OS         | ✅                |
| 4    | RISC-V64 | Milk-V Megrez      | 32 GB  | NVMe         | EIC7700        | 4     | MMU      | RV64GCV (1.0)  | Very High  | Advanced OS     | ✅                |
| 4    | RISC-V64 | Sophgo SG2042      | 64 GB  | NVMe         | 64-core        | 64    | MMU      | RV64GC         | Server     | Distributed OS  | ✅                |

---

## Support Boundary Rules

### Hard Cut Line for Bharat-OS Runtime

| Capability  | Runtime Reality |
| ----------- | --------------- |
| MPU-only    | Kernel experiments only (no full OS runtime) |
| MMU-lite    | Limited OS profile, weak isolation guarantees |
| Full MMU    | Real Bharat-OS deployable runtime |

**Conclusion:** practical Bharat-OS runtime targets start at **Tier 2 with full MMU**, while Tier 0–1 remains valid for micro-profile experimentation.

---

## Bharat-OS Hardware Classes

| Class | Intent | Typical Boards |
| ----- | ------ | -------------- |
| H0 | Experimental micro-kernel slice | STM32, CH32V003, GD32 |
| H1 | Embedded limited runtime | ESP32-C6, STM32MP1-style hybrids |
| H2 | Edge minimal deployable runtime | Duo S, Raspberry Pi 3 |
| H3 | Mainline desktop/mobile targets | Pi 5, VisionFive 2, Rock Pi 5 |
| H4 | Advanced AI/cloud/server targets | Jetson, Megrez, SG2042 |

### Profile Mapping by Tier

| Tier | Suggested Profile |
| ---- | ----------------- |
| 0–1  | `EMBEDDED_RT` / `EMBEDDED_MINIMAL` |
| 2    | `EDGE_MINIMAL` / `IOT_EDGE` |
| 3    | `DESKTOP` / `MOBILE` |
| 4    | `CLOUD` / `AI` / `AUTOMOTIVE` |

---

## H0 Policy (Important)

H0 is a **first-class Bharat-OS target**, but **not** a full Bharat-OS runtime.

H0 permits:

- boot and trap/interrupt handling
- timer and tiny scheduler
- static memory regions + MPU isolation
- static IPC/event primitives
- fixed board wiring (no dynamic discovery)

H0 explicitly excludes:

- virtual memory manager
- dynamic service supervisor
- rich driver discovery model
- full networking/filesystem stacks
- personality layer (Linux/Android compatibility)

This prevents false claims such as “MPU board supported” being misread as “full OS supported.”

---

## Build-Time Contract

Use capability classes directly in build configuration:

```bash
BHARAT_HW_CLASS=H3
BHARAT_PROFILE=DESKTOP
BHARAT_ARCH=ARM64
```

This keeps build outputs aligned with runtime reality and avoids over-promising support.

---

## Recommended CSV Schema for Agent-Safe Classification

```csv
board_name,arch,ram_bytes,storage_bytes,memory_protection,memory_translation,core_count,bharat_hw_class,bharat_support_level,full_os_runtime,supported_profiles,service_model,driver_model,notes
```

### Example Rows

```csv
CH32V003,riscv32,2048,16384,MPU,NONE,1,H0,EXPERIMENTAL,false,EMBEDDED_MINIMAL,STATIC_ONLY,STATIC_BOARD_WIRING,"Kernel bring-up, MPU isolation only"
STM32F4 Black Pill,arm32,131072,524288,MPU,NONE,1,H0,EXPERIMENTAL,false,EMBEDDED_MINIMAL,STATIC_ONLY,STATIC_BOARD_WIRING,"Tiny kernel only"
ESP32-C6,riscv32,524288,4194304,MPU,NONE,1,H1,LIMITED,false,EMBEDDED_RT,TINY_STATIC,STATIC_BOARD_WIRING,"Possible richer RT profile"
Milk-V Duo S,arm64,536870912,8589934592,MMU,FULL,1,H2,REAL,true,EDGE_MINIMAL,LIGHT_SERVICES,BOARD_PLUS_CLASS,"Practical low-end Bharat-OS target"
Raspberry Pi 5,arm64,17179869184,0,MMU,FULL,4,H3,REAL,true,DESKTOP_EDGE,FULL_SERVICES,CLASS_BASED,"Mainstream Bharat-OS target"
```

---

## Agent Classification Rule (Reference)

```python
if memory_translation == "FULL_MMU":
    full_os_runtime = True
elif memory_protection == "MPU" and ram_bytes <= 262144:
    bharat_hw_class = "H0"
    bharat_support_level = "EXPERIMENTAL"
    full_os_runtime = False
elif memory_protection == "MPU" and ram_bytes > 262144:
    bharat_hw_class = "H1"
    bharat_support_level = "LIMITED"
    full_os_runtime = False
else:
    manual_review = True
```

---

## Final Recommendation

Adopt a **capability-class support policy**:

- **H0** = micro-kernel experimental slice
- **H1** = limited embedded runtime
- **H2+** = real deployable Bharat-OS runtime

This preserves architectural integrity (mechanism in kernel, policy in services, behavior via profiles) and creates a transparent, automation-friendly support model.
