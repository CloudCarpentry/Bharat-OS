---
title: "Bharat-OS GUI Shell Demo"
status: "experimental"
owner: "Experience Working Group"
last_updated: "2026-08-12"
tags: [gui, shell, qemu]
see_also:
  - "docs/architecture/display_subsystem.md"
  - "docs/architecture/boot/boot_display_architecture.md"
---

# Bharat-OS GUI Shell Demo (DEMO-P0-001)

The first GUI shell turns the existing LVGL path into an operating-system-shaped
demo. It deliberately remains an experience-layer application: the kernel exports
mechanisms and capabilities, display and input adapters connect those mechanisms to
LVGL, and the shell owns navigation, presentation, and device-label policy.

## Demonstrated flow

```text
Boot -> Bharat-OS splash -> Desktop / Launcher
                              |-> System
                              |-> Devices
                              `-> Demo Apps
```

The launcher exposes System, Devices, Processes, Network, Hardware & Sensors, and
Demo Apps entries. This P0 release implements four distinct views: splash,
launcher, system information, and device viewer; not-yet-service-backed entries
open a common demo-app view rather than claiming live subsystem data.

The System view shows architecture, CPU count, memory, profile, runtime, kernel
build, capability flags, live monotonic uptime, and heap counters. The shell model
uses explicit demo defaults today and has a snapshot-provider boundary for a future
capability-mediated system information service. The Device view similarly labels
the six QEMU demo devices; `READY` is target-demo state, not a claim of production
driver maturity.

## Interaction

Mouse movement and button events are consumed by the LVGL pointer device. Keyboard
Tab/arrows move focus, Enter activates the focused item, and Escape is translated
for LVGL navigation. The focused launcher buttons and Back button share one LVGL
navigation group.

## QEMU demo target

Build, package, and run the dedicated 512 MiB x86_64 graphical target:

```bash
./tools/build.sh all --target-yaml delivery/targets/qemu/x86_64_showcase_gui.yaml --smoke
python3 tools/build.py run --target-yaml delivery/targets/qemu/x86_64_showcase_gui.yaml
```

Expected serial milestones include `[gui] display-ready width=<w> height=<h>`
and `[gui] first-frame-presented`. To prove pixels rather than only control-flow
progress, run the visual gate:

```bash
python3 tools/test/qemu_gui_smoke.py \
  --target delivery/targets/qemu/x86_64_showcase_gui.yaml
```

After validating the initial scanout, the harness injects a QMP Tab key event,
waits for `[gui] input-observed`, captures `before-input.ppm` and
`after-input.ppm`, and rejects both an unchanged image and an implausible
whole-frame transition. Input service discovery fails closed; there is no direct
QEMU-device fallback in the showcase.

The harness retains the QMP screenshot and serial/QEMU logs under
`build/x86_64-dev/artifacts/gui-smoke/`. It fails when the captured dimensions
disagree with the broker-queried mode or when scanout is blank/uniform.

## Boundaries and limitations

- The native showcase now exercises display-broker v2 session, lease, surface,
  buffer, and present operations. Its client mapping is still process-local
  until kernel-mediated shared-buffer mapping and the x86_64 scanout backend are
  complete; consequently the visual smoke gate is expected to expose, rather
  than conceal, a uniform or stale QEMU scanout.
- The showcase has a fail-closed input-service client, but physical events remain
  unavailable until the service runtime publishes `inputmgr` and owns the
  VirtIO-input routing path.
- Runtime data is intentionally injected through a provider. The UI never reads
  kernel globals or driver internals.
- Process, network, hardware, and sensor views are placeholders for later
  capability-mediated service clients.
