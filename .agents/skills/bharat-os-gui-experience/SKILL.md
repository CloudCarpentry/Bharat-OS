---
name: bharat-os-gui-experience
description: Apply when working on display drivers, framebuffers, GUI stacks, compositors, input streams, touch/pointer devices, or visual smoke testing.
---

# Bharat-OS GUI & Display Experience Skill

## Architectural Foundations

Bharat-OS strictly enforces userspace visual presentation under **ADR-035 (Tiered Display Architecture & Zero-Bloat Policy)** and **ADR-034 (Capability-Mediated GUI Input Stream)**:

1. **Kernel Zero-Bloat Boundary**:
   - The microkernel is strictly limited to:
     - Boot video metadata collection (`boot_video.c`).
     - Physical framebuffer MMIO page mapping (`boot_video_map.c`).
     - Capability minting and delegation (`CAP_TYPE_MEMORY` / `CAP_DISPLAY_FB`).
   - **In-kernel software pixel rendering, font bitmaps, splash widgets, and window management are strictly prohibited in `core/kernel/`.**
   - All rendering, composition, fonts, and widget layouts belong exclusively in userland services and stacks (`core/services/` and `core/stacks/ui/`).

2. **The 4 Display Tiers**:
   - **Tier 0 (Headless / Zero-Display)**:
     - Targeted at Automotive ECUs, IoT sensors, headless cloud VMs, and server profiles.
     - `BHARAT_BOOT_GUI=OFF`, `BHARAT_ENABLE_UI=OFF`. Display code is completely compiled out (0 bytes overhead). Diagnostic output routes via serial UART and telemetry ring buffers.
   - **Tier 1 (Tiny UI / Constrained Display)**:
     - Targeted at MPU / MMU-Lite embedded appliances and basic instrument clusters.
     - Uses `core/stacks/ui/lcd/tiny_ui.c`. Bounded footprint (< 16 KB RAM) with **zero dynamic heap allocations** (`malloc`-free).
   - **Tier 2 (Embedded Rich UI)**:
     - Targeted at MMU-Lite / MMU automotive infotainment and smart mobile displays.
     - Uses the LVGL v9 adapter (`core/stacks/ui/adapters/lvgl/`). Supports single/double buffering and touch/pointer input via `inputmgr`.
   - **Tier 3 (Advanced Composited Desktop)**:
     - Targeted at full MMU workstations and Android/Linux personalities.
     - Employs compositor service (`compositord`), display manager (`displayd`), and VirtIO-GPU / DRM drivers. Surfaces are capability-governed across isolated client processes.

3. **Capability-Mediated Input Stream (ADR-034)**:
   - `inputmgr` owns the normalized, bounded input queue.
   - Applications discover `inputmgr` through nameservice and invoke bounded drains (at most 16 fixed-width events per invocation).
   - Clients receive fixed-width by-value events (`uint32_t`, `int16_t`). **Never pass raw driver pointers, device registers, or shared queue memory.**

## Canonical GUI Targets by Device Profile

| Profile | Target YAML | Display Hardware | Primary Resolution |
|---|---|---|---|
| **Desktop x86_64** | `delivery/targets/qemu/x86_64_desktop_gui.yaml` | Standard VGA / VirtIO-GPU | 1024x768x32 |
| **Desktop ARM64** | `delivery/targets/qemu/arm64_desktop_gui.yaml` | VirtIO-GPU MMIO / PL111 | 1024x768x32 |
| **Desktop RISC-V 64** | `delivery/targets/qemu/riscv64_desktop_gui.yaml` | VirtIO-GPU PCI | 1024x768x32 |
| **Mobile ARM64** | `delivery/targets/qemu/arm64_mobile_gui.yaml` | VirtIO-GPU Touchscreen | 720x1280x32 (Portrait) |
| **Mobile RISC-V 64** | `delivery/targets/qemu/riscv64_mobile_gui.yaml` | VirtIO-GPU Touchscreen | 720x1280x32 (Portrait) |
| **Edge / Instrument Cluster** | `delivery/targets/qemu/arm32_edge_gui.yaml` | SimpleFB / LCD Controller | 800x480x16 |

## Verification & Smoke Testing Procedure

1. **Check Zero-Bloat Policy**:
   Verify that all target configurations adhere to ADR-035 display tiers:
   ```bash
   python3 tools/lint/check_display_tiers.py
   ```

2. **Execute Automated GUI Smoke Matrix**:
   Run graphical smoke testing across canonical desktop and mobile profiles:
   ```bash
   python3 tools/agents/verify.py --gui-smoke
   ```

3. **Expected Visual Runtime Markers**:
   Under QEMU, visual boot must emit:
   - `DISPLAY_CONTROLLER_BOUND` (Hardware display device mapped)
   - `SPLASH_VISIBLE` (Boot display daemon has drawn initial frame)
   - `INPUTMGR_READY` (Input queue initialized and listening)
   - `GUI_INTERACTIVE` (Main event loop processing input events)
