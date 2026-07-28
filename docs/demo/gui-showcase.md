---
title: "Bharat-OS Live Graphical System Dashboard Showcase"
status: "baseline"
owner: "Jules"
last_updated: "2026-03-24"
tags:
  - "gui"
  - "showcase"
  - "framebuffer"
  - "dashboard"
see_also:
  - "docs/architecture/display_subsystem.md"
---

# Bharat-OS Live Graphical System Dashboard Showcase (DEMO-P2-001)

This document describes the design, execution, and verification of the Live Graphical System Dashboard for Bharat-OS, running on a real memory-mapped QEMU/q35 framebuffer.

## What the Demo Proves

The `DEMO-P2-001` showcase provides an end-to-end, high-contrast, interactive proof of the Bharat-OS display and UI subsystem integration. Specifically, it validates:

1. **Boot Framebuffer → Generic Display Registry Bridge**: The real, boot-loader allocated, validated, and mapped framebuffer is successfully registered into the generic display registry as a standard `bharat_display_device_t` pointing to the real `virt_addr`, with zero dynamic allocations.
2. **Format-Safe Translation**: Explicitly maps and validates boot pixel-formats (`ARGB8888`, `XRGB8888`, `RGB565`) to generic display API formats with strict overflow-safe checks (`height * stride <= size`) to prevent memory corruption.
3. **High-Contrast FBUI Dashboard**: Renders a polished dark/navy system dashboard with Indian saffron accents, green success highlights, readable 8x16 font rendering, and realistic cards, progress bars, and checkboxes.
4. **Interactive State Update Loop**: Dispatches a deterministic synthetic touch event against the "Run Demo" button which updates the event pipeline state, toggles the "Event Dispatch" checkbox, and refreshes/re-renders the dashboard layout live.
5. **Deterministic Serial Output Verification**: Emits machine-readable serial markers that confirm success at each stage only when rendered through the real framebuffer.

---

## Supported Demo Target

- **Architecture**: `x86_64`
- **Machine**: QEMU `q35`
- **Target Configuration**: `delivery/targets/qemu/x86_64_showcase_gui.yaml`

---

## Build Command

Configure and build the dedicated showcase target using the repository target runner:

```bash
python3 tools/build.py configure --target-yaml delivery/targets/qemu/x86_64_showcase_gui.yaml
python3 tools/build.py build --target-yaml delivery/targets/qemu/x86_64_showcase_gui.yaml
```

---

## Run Command

Package and execute the showcase target under QEMU:

```bash
python3 tools/build.py package --target-yaml delivery/targets/qemu/x86_64_showcase_gui.yaml
python3 tools/build.py run --target-yaml delivery/targets/qemu/x86_64_showcase_gui.yaml
```

---

## Expected Graphical Result

When QEMU launches with the graphical window enabled, you will observe the following visual sequence:

1. **Early Boot Splash**: Bharat-OS boot splash is rendered on the early framebuffer.
2. **Dashboard Transition**: The screen transitions to a beautiful, dark-navy (`0xFF0A1128`) themed Live Graphical Dashboard:
   - **Top Saffron Accent Bar**: Thick horizontal line at the very top.
   - **Header Branding**: "BHARAT-OS" in saffron text on the left, and a green "SYSTEM ONLINE" label on the right.
   - **Two Information Cards**: Left card lists system properties (Architecture, Profile, Execution, Personality), and right card lists platform readiness statuses.
   - **Progress Bar**: Saffron progress label and filled green bar showing `100%` completion.
   - **Controls & Checkboxes**: Under the buttons, three checkboxes: `[✓] Framebuffer`, `[✓] Widgets`, and `[✓] Event Dispatch`.
3. **Interactive Update**: Upon synthetic dispatch, `[ ] Event Dispatch` changes to `[✓] Event Dispatch` and the label transitions to `Event Pipeline: PASS` to confirm full event routing and state re-render.

---

## Expected Serial Markers

The following deterministic machine-readable markers are printed to the serial console (`stdio`):

```text
BHARAT_GUI_DEMO:START
BHARAT_GUI_DEMO:DISPLAY=REAL
BHARAT_GUI_DEMO:RENDER=PASS
BHARAT_GUI_DEMO:EVENT=PASS
BHARAT_GUI_DEMO:COMPLETE
```

*(Note: `COMPLETE` is never emitted for dummy/offscreen memory fallbacks).*

---

## Roadmap & Limitations

### Demonstrated Today
- Boot-loader framebuffer mapping and exposure to generic display registry.
- Static, allocation-free device registrations.
- Standalone FBUI rendering, clipping, layout grid, and event propagation.
- Deterministic synthetic interaction testing.

### Future Window System Roadmap
- Real hardware graphics/GPU drivers (e.g. virtio-gpu, DRM/KMS).
- Desktop compositors and multi-window managers.
- Hardware-accelerated rendering and page-flipping.
- Multi-user input drivers (USB HID, real PS/2 mouse and keyboard events).
- Capability-gated display leasing and hardware resource access.
