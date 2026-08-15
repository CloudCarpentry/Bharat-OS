---
title: GUI Product P0 Plan
status: Active
owner: UI and Display Working Group
last_updated: 2026-08-13
---

# GUI product P0 plan

## Lane and evidence rule

This is a Demo/Product lane. It does not replace capability, service-runtime, or
kernel-correctness P0 work. A serial message proves only the state named by that
message; visual completion additionally requires a QEMU screenshot captured from
scanout. Generated screenshots and logs are CI artifacts and are not source files.

The display broker owns display enumeration, leases, surfaces, buffers, and
presentation. Applications render only into mappings granted for
compositor-owned buffers. They must not map physical scanout or call a platform
display backend directly. Every operation fails closed on nameservice, transport,
handle-generation, rights, mode, or presentation-status failure.

## GUI-001 — x86_64 first-frame vertical slice

`gui_showcase` now discovers display broker v2 through nameservice, enumerates a
display, queries its mode, requests `LEASE | WRITE | PRESENT`, creates a surface,
registers and attaches buffers, and asks the broker to present. The application
does not manufacture display, lease, surface, or buffer handles. It emits
`[gui] display-ready` only after mode and lease validation and emits
`[gui] first-frame-presented` only after the broker reports a nonzero frame counter
and the expected active buffer.

The remaining system-wide closure condition is deliberately separate from that
broker acknowledgement: the display broker's current buffer contract has no
kernel-mediated shared-buffer mapping or x86_64 scanout backend. Until those
authorities exist, the LVGL client mapping is process-local and the marker is not
proof that QEMU received pixels. GUI-002 must therefore reject a uniform
screenshot even when the marker is present. The architecture must not be bypassed
with a raw framebuffer pointer.

## GUI-002 — QEMU visual smoke harness (implemented; visual gate pending)

`tools/test/qemu_gui_smoke.py` uses the canonical target
`delivery/targets/qemu/x86_64_showcase_gui.yaml` as its default. The host unit
suite covers command construction, marker timeout and parsing, malformed PPM,
dimension mismatch, uniform and successful frames, and forced termination.

1. Validate and build exclusively from the target YAML.
2. Start QEMU with a serial log and a private QMP socket.
3. Wait with a bounded monotonic deadline for `[gui] display-ready` and
   `[gui] first-frame-presented`.
4. Issue QMP `screendump` and parse the resulting PPM without optional image
   packages.
5. Require the queried/captured dimensions to agree and reject blank or uniform
   frames using documented pixel-diversity thresholds.
6. Preserve the screenshot and serial log under the selected build artifact
   directory, shut QEMU down through QMP, and return a stable CI exit status.
7. Test command construction, timeout, malformed PPM, wrong dimensions, uniform
   frame, successful frame, and forced QEMU termination with host unit tests.

**Gate:** one command builds, boots, captures a non-uniform real scanout frame,
and exits successfully. A serial-only pass is forbidden.

## GUI-003 — input round trip (P0/P1, client and harness implemented)

Replace the zero-event showcase provider with the input-service client. Preserve
the driver -> input service -> LVGL adapter -> shell boundary; the application
must not read QEMU devices directly. Extend GUI-002 to inject a key or pointer
event through QMP, capture a second frame, and require a bounded visible change in
the intended HOME control. Add malformed, unauthorized, queue-full, stale-device,
and no-change tests.

**Gate:** boot -> frame -> injected input -> visibly changed frame, with before
and after artifacts.

The showcase now resolves the versioned `inputmgr` endpoint through nameservice
and drains a bounded, fixed-width event response over IPC; lookup, transport,
version, status, size, and count failures produce no events. The LVGL adapter
accepts only supported relative axes, the primary pointer button, and navigation
keys, and emits `[gui] input-observed` on the first accepted event. The QMP smoke
harness sends a Tab press/release, waits for that guest marker, retains before and
after PPM files, and requires a configurable bounded pixel delta.

The end-to-end gate remains pending until the independently tracked input service
runtime owns and publishes the `inputmgr` endpoint and the x86_64 VirtIO input
driver routes its queue into that service. The application deliberately does not
fall back to direct device access or an in-process input queue when discovery
fails.

## GUI-004 — readiness-driven splash state machine (P1)

Implement the service-owned sequence `STATIC_BOOT_LOGO -> DISPLAY_READY ->
SPLASH_ANIMATION -> SHELL_READY -> HOME`. Display and shell readiness are events,
not delays. Decoration never blocks boot: early readiness shortens the transition,
while delayed readiness lets animation continue within a bounded loop. Target a
500–900 ms animation after graphics readiness and add early-ready, late-ready,
timeout, and degraded-display tests.

## GUI-005 — versioned brand asset pipeline (P1)

Adopt one reviewed SVG master plus symbol, horizontal, monochrome, dark/light,
and splash-safe variants. Record clear space, minimum size, typography, and
semantic theme tokens. Generate PNG/raw/C runtime assets in the build directory;
do not hand-maintain embedded byte arrays or commit generated output. Brand
review must verify monochrome recognition, licensing/provenance, and avoidance of
government-emblem confusion.

## GUI-006 — one UI path and architecture qualification (P1/P2)

Declare the shell/LVGL/display-broker path authoritative, migrate remaining
consumers, then quarantine or remove `experience/user/ui/fbui`. Add a layer/build
check preventing applications from selecting competing display authorities.
Parameterize GUI-002 for ARM64 only after x86_64 meets its visual gate; qualify
RISC-V64 after its real framebuffer/platform backend is available. Unsupported
targets fail explicitly rather than silently falling back to a shadow buffer.

## Delivery order

1. Close the shared-buffer and x86_64 scanout gap exposed by GUI-001.
2. Deliver GUI-002 and make non-uniform scanout the first product gate.
3. Deliver GUI-003 and make before/after interaction the second product gate.
4. Add GUI-004 and GUI-005 without weakening either gate.
5. Complete GUI-006 and then extend the proven harness per architecture.
