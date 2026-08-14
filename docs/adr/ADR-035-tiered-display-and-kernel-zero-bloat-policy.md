---
title: Tiered Display Architecture and Kernel Zero-Bloat Policy
status: Accepted
date: 2026-08-14
---

# Context

Bharat-OS targets a diverse spectrum of devices, from resource-constrained automotive ECUs, safety appliances, and MPU/MMU-Lite microcontrollers (which may have no display or only a basic character/segment screen) to rich MMU workstations and infotainment systems requiring high-performance compositing and window management.

Historically, experimental code introduced in-kernel software rendering and font bitmaps (`core/kernel/src/display/boot_gui.c`), risking kernel bloat and violating the capability microkernel invariant that the kernel contains mechanisms only, while policy, layout, fonts, and rendering remain in userspace.

# Decision

1. **Kernel Mechanism Boundary**:
   - The kernel display subsystem is strictly restricted to:
     - Boot video metadata collection and validation (`boot_video.c`).
     - Physical framebuffer MMIO memory mapping (`boot_video_map.c`).
     - Capability minting and delegation (`CAP_TYPE_MEMORY` / `CAP_DISPLAY_FB`) via `display_handoff.c`.
   - In-kernel pixel rendering, bitmap fonts, splash widgets, and window management are strictly forbidden in `core/kernel/`.

2. **4-Tier Display Capability Model**:
   - **Tier 0 (Headless / Zero-Display)**: Automotive ECUs, IoT sensors, servers, cloud VMs.
     - `BHARAT_BOOT_GUI=OFF`, `BHARAT_ENABLE_UI=OFF`, `BHARAT_ENABLE_SERVICE_BOOT_DISPLAYD=OFF`, `BHARAT_UI_LVGL=OFF`.
     - Kernel display code is completely compiled out (0 bytes overhead). Diagnostic output routes via serial UART and telemetry ring buffers.
   - **Tier 1 (Tiny UI / Constrained Display)**: MPU / MMU-Lite appliances, basic instrument clusters.
     - Userspace `boot_displayd` uses `core/stacks/ui/lcd/tiny_ui.c`.
     - Statically bounded memory footprint (< 16 KB RAM), zero dynamic heap allocations (`malloc`-free).
   - **Tier 2 (Embedded Rich UI)**: MMU-Lite / MMU, automotive infotainment, smart displays.
     - Userspace UI uses the LVGL v9 adapter (`core/stacks/ui/adapters/lvgl/`). Single or double-buffered with pointer/touch input via `inputmgr`.
   - **Tier 3 (Advanced Composited Desktop)**: Full MMU, multi-window, Android/Linux personalities.
     - Compositor service (`compositord`), display manager (`displayd`), DRM/KMS or VirtIO-GPU driver services. Surface composition is capability-governed across isolated client processes.

3. **Compile-Time & Linting Enforcement**:
   - `tools/lint/check_layer_references.py` forbids inclusion of UI rendering/font headers in `core/kernel/`, `core/hal/`, and `core/arch/`.
   - `tools/lint/check_display_tiers.py` verifies all target configurations in `delivery/targets/` satisfy the tier policies.

# Consequences

- Automotive ECU and headless profiles incur zero kernel bloat or display overhead.
- All visual presentation is safely isolated in userspace behind capability and memory leases.
- Embedded devices (MPU/MMU-Lite) can render clean boot and diagnostic screens with deterministic, static memory bounds.
