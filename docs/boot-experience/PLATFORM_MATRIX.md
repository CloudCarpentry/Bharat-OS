# Bharat-OS Cross-Architecture Display & Device Profile Matrix

**Date**: 2026-10-10  
**Specification**: PLATFORM-UX-004  
**Profiles Supported**: `FULL`, `COMPACT`, `HEADLESS`  

## 1. Device Profile Definitions

Bharat-OS defines three distinct device profile classes for boot and display experiences:

1. **`FULL`**:
   - **Target Class**: Desktop, workstation, capable automotive cockpit, tablet/mobile.
   - **Visuals**: Full animated or branded splash, LVGL rich graphical environment, multi-window shell, interactive graphical boot diagnostics.
   - **Input**: Multi-touch / Pointer + Full Hardware Keyboard.
   - **Display Requirement**: Hardware-accelerated or VirtIO-GPU / Bochs framebuffer with lease broker.

2. **`COMPACT`**:
   - **Target Class**: Embedded display, wearable, IoT panel, industrial status screen.
   - **Visuals**: Lightweight `tiny_ui` framebuffer splash, static branding, compact status/progress bar, essential error indicator.
   - **Input**: Pushbutton, rotary encoder, simple key navigation (`NEXT`, `PREV`, `SELECT`, `BACK`).
   - **Display Requirement**: Linear raw framebuffer / LCD controller, zero dynamic heap overhead.

3. **`HEADLESS`**:
   - **Target Class**: Server blade, headless embedded node, automotive gateway / ECU, early bringup.
   - **Visuals**: Serial console diagnostics, structured boot telemetry markers (`[BOOT]`, `[HAL]`, `[MM]`, `[SCHED]`). Zero framebuffer dependency.
   - **Input**: Serial UART / IPC commands.
   - **Display Requirement**: None. Display initialization failure or absence is non-fatal and fail-safe.

---

## 2. Five-Architecture Platform Matrix

| Architecture | Canonical Target YAML | Profile Class | Display Backend | Input Backend | Splash Support | GUI Diag Support | QEMU Build | QEMU Smoke Status | Notes / Blockers |
|---|---|---|---|---|---|---|---|---|---|
| **x86_64** | `x86_64_desktop_gui.yaml` | `FULL` | VirtIO-GPU / Bochs / Memory Framebuffer | PS/2, VirtIO Input | Yes (Branded + LVGL) | Yes (Interactive) | **PASS** | **PASS** | Primary interactive desktop & showcase platform |
| **x86_64 (Headless)** | `x86_64_desktop_headless.yaml` | `HEADLESS` | None (Serial console) | 16550 UART | Fallback (Serial markers) | Fallback (Serial stream) | **PASS** | **PASS** | Verified in automated CI matrix |
| **ARM64** | `arm64_desktop_gui.yaml` | `FULL` | VirtIO-GPU MMIO / RAMFB | VirtIO Input | Yes (Branded + LVGL) | Yes (Interactive) | **PASS** | **PASS** | QEMU `virt` machine with VirtIO-GPU |
| **ARM64 (Headless)** | `arm64_desktop_headless.yaml` | `HEADLESS` | None (PL011 Serial) | PL011 UART | Fallback (Serial markers) | Fallback (Serial stream) | **PASS** | **PASS** | Verified in automated CI matrix |
| **ARM32** | `arm32_mmu_lite_headless.yaml` | `HEADLESS` | Serial (PL110 planned) | PL011 UART | Fallback (Serial markers) | Fallback (Serial stream) | **PASS** | **BLOCKED** | Prerequisites: Independent PMM boot resolution in progress |
| **RISC-V64** | `riscv64_desktop_gui.yaml` | `FULL` | VirtIO-GPU MMIO | VirtIO Input | Yes (Branded + LVGL) | Yes (Interactive) | **PASS** | **PASS** | QEMU `virt` machine |
| **RISC-V64 (Headless)** | `riscv64_desktop_headless.yaml` | `HEADLESS` | None (16550 UART) | 16550 UART | Fallback (Serial markers) | Fallback (Serial stream) | **PASS** | **PASS** | Verified in automated CI matrix |
| **RISC-V32** | `riscv32_mmu_lite_headless.yaml` | `HEADLESS` / `COMPACT` | Serial / Linear FB (optional) | 16550 UART | Fallback (Serial markers) | Fallback (Serial stream) | **PASS** | **PASS** | Resource-constrained embedded target |

---

## 3. Fail-Closed and Graceful Fallback Guarantee

1. **Kernel Boot Decoupling**: The kernel boot sequence (`core/kernel/src/kernel_boot.c`) never blocks on display availability. If `runtime_try_boot_video()` fails to detect or map a valid framebuffer, it logs `Video handoff inactive or invalid. Booting serial UI fallback only` and continues execution into userspace.
2. **Display Daemon Failover**: `boot_displayd` detects whether `bh_showcase_display_open()` succeeds. If unavailable, it transitions to `BOOT_STATE_UNAVAILABLE` and terminates cleanly with exit code 0 without crashing system startup.
3. **Renderer Failover**: If LVGL initialization fails or is disabled at compile time, `boot_displayd` falls back to the deterministic zero-heap `tiny_ui` framebuffer renderer.
