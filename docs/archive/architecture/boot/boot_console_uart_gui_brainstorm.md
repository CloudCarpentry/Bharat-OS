# Boot-Level Display, Console, UART, and Basic GUI: Architecture Brainstorm

## 1) Problem Statement and Goals

Design a boot-to-kernel visual and I/O stack that supports:

- Multiple console backends (serial/UART, text console, framebuffer console, optional graphics console).
- Early boot diagnostics on the widest hardware set.
- Basic GUI splash at boot with OS logo + kernel version string.
- Runtime selection based on hardware architecture and board capabilities.
- Clear split of responsibilities across bootloader, kernel, drivers, service layer, and user stack.

Primary goals:

1. **Always have output** (fail-safe serial path).
2. **Progressive enhancement** (serial -> text -> framebuffer -> GUI).
3. **Deterministic boot** (strict fallback order and timeout model).
4. **Portable architecture** (x86_64, ARM64, RISC-V, etc.).
5. **Branding + diagnostics** (logo and version without sacrificing reliability).

---

## 2) Layering Model (Who Owns What)

## Bootloader responsibilities

Own only minimal, hardware-agnostic setup needed to start kernel safely.

- CPU bring-up basics, memory map handoff, initrd handoff, device-tree/ACPI handoff.
- Early console init (prefer serial first).
- Optional framebuffer mode setup (GOP/UEFI, simplefb, or platform video init).
- Pass a `BootDisplayInfo`/`BootIOInfo` handoff structure to kernel:
  - active console type
  - framebuffer address/stride/pixel format
  - UART base address/clock/baud if known
  - board/model identifiers
  - architecture identifier
  - secure boot / trusted mode bits if relevant
- Optional static splash draw in bootloader **only if** it does not block boot.

**Do not put policy-heavy logic in bootloader** (theme engines, full compositor, complex input stacks).

## Kernel responsibilities

Kernel owns correctness, fallback policy, and synchronization.

- Parse boot handoff and discover devices.
- Register console framework with priorities and states (`early`, `normal`, `panic`).
- Maintain ring buffer for logs; replay to newly available consoles.
- Early printk/early log pipeline and promotion to full console.
- Framebuffer console core (text over pixels) and optional simple splash API.
- Panic path that can print to serial + framebuffer even if higher stack fails.
- Export stable syscalls/ioctls for user-space display service (if GUI at boot extends beyond kernel).

## Driver responsibilities

Drivers own hardware-specific control.

- UART drivers: init, TX/RX FIFO, interrupt/polled modes, clock and baud setup.
- Display drivers:
  - early/simple framebuffer (no modeset)
  - full KMS/modesetting path later
- Input drivers if needed for boot menu (keyboard, gpio keys, touchscreen minimal).
- Board glue drivers for clocks, reset lines, pinmux needed by UART/display.

## Service responsibilities (user-space early services)

Early-boot services handle richer behavior not ideal in kernel.

- `boot-uxd` (example): draws animated splash/logo/progress once display stack is ready.
- `log-forwarder`: consumes kernel log buffer and writes to persistent storage + optional on-screen debug overlay.
- Hardware profile resolver: chooses theme/assets based on board SKU or product flavor.
- Transition manager: clean handoff from splash to login/launcher.

## Stack/Application responsibilities

Higher-level UX and product logic.

- Branding pack (logos, color themes, version presentation rules).
- Localization of boot messages.
- Policy for silent boot vs verbose boot.
- Developer mode toggles (show kernel logs on screen, show boot timing).

---

## 3) Capability Matrix by Platform Type

## Architecture-level abstraction

Create `arch_boot_caps` generated at early kernel init:

- `has_efi_gop`
- `has_dt_simplefb`
- `has_legacy_vga`
- `has_mmio_uart`
- `has_pci_gpu`
- `secure_boot_locked`
- `trusted_display_path`

Then derive runtime modes:

1. **Serial-only mode**: headless/server boards.
2. **Text + serial mode**: legacy text consoles.
3. **Framebuffer + serial mode**: embedded/mobile/modern UEFI.
4. **Framebuffer + GUI splash + serial mirror**: preferred default for consumer boards.

## Board-level overlay

Board descriptor table (DT/ACPI + board DB):

- preferred display output (HDMI, DSI, LVDS, eDP)
- logo safe area and native resolution hints
- UART debug port mapping
- boot silence policy
- power constraints (skip animation on low battery)

This allows same kernel image to adapt behavior by board profile.

---

## 4) Console Framework Design

Use a **multi-sink console manager** in kernel.

Each sink implements:

- `probe()`
- `init(stage)`
- `write(level, text)`
- `flush()`
- `panic_write()`
- `suspend()/resume()`

Sink types:

- `serial_sink`
- `text_sink` (if hardware text mode exists)
- `fb_text_sink`
- `boot_gui_sink` (very limited drawing primitives)

Features:

- Priority and fanout policy:
  - panic: serial + fb_text forced
  - normal: chosen default + optional mirror to serial
- Log level gating by sink (e.g., GUI shows info+, serial shows debug+).
- Rate limiting to avoid boot stalls on slow UART.

---

## 5) UART Strategy

## Why UART is non-negotiable

UART is the universal fallback and factory/debug lifeline.

Implementation ideas:

- Early polled UART in boot + early kernel.
- Promote to interrupt-driven after scheduler init.
- Optional DMA in full driver stage for throughput.
- Config chain for baud/port:
  1. bootloader-provided config
  2. board descriptor
  3. kernel cmdline override
  4. hard default

Add robust behavior:

- watchdog-safe output path (bounded blocking)
- panic-time direct register writes (bypass locks)
- optional timestamp prefix for boot profiling

---

## 6) Framebuffer and Basic GUI at Boot

## Rendering phases

1. **Phase A: static splash** (bootloader or early kernel)
   - draw logo centered
   - draw kernel version string bottom-left or bottom-center
2. **Phase B: progress state** (kernel service)
   - spinner/progress bar from milestone events
3. **Phase C: handoff**
   - smooth fade or immediate replace by display manager/compositor

## Minimal kernel GUI primitive set

Keep kernel graphics tiny:

- `fill_rect`
- `blit_rgba`
- `draw_mono_glyph`
- optional `alpha_blend` (can be disabled for tiny targets)

Anything advanced (font shaping, animation scripting, image formats beyond raw/simple) should remain in user-space service.

## Asset strategy

- Store a tiny fallback logo in kernel (RLE/raw indexed).
- Prefer board/product-provided assets in initrd.
- Signature/verification if secure boot chain requires trusted assets.

## Kernel version display policy

Display format example:

`BharatOS Kernel vX.Y.Z (arch-board)`

Source of truth:

- compile-time version macros + git describe (if enabled)
- append board and arch from runtime detection

Need truncation rules for low-resolution displays.

---

## 7) Boot Policy Engine (Decision Flow)

Pseudo flow:

1. Initialize early UART and early logger.
2. Parse handoff (DT/ACPI/boot info).
3. Detect display capability.
4. If framebuffer available, init `fb_text_sink`.
5. If splash enabled and not in verbose/debug mode, draw logo + version.
6. Continue boot milestones; update simple progress indicator.
7. Start user-space `boot-uxd` service for richer splash if available.
8. On service timeout/failure, keep kernel fallback display.
9. Handoff to compositor/login; unregister boot GUI sink.

Cmdline toggles:

- `boot.ui=off|text|splash|full`
- `boot.serial=on|off|mirror`
- `boot.verbose=0|1`
- `boot.logo=<asset_id>`

---

## 8) Failure, Recovery, and Debug Model

## Failure classes

- no display detected
- display init fails after mode set
- logo asset missing/corrupt
- boot GUI service crashed
- deadlock risk in console locks

## Recovery rules

- Always keep serial available.
- If graphics path fails, auto fallback to text or serial without reboot.
- If panic occurs, force flush logs to serial + basic fb text overlay.
- Keep last N KiB boot log in reserved memory for post-mortem.

## Developer diagnostics

- boot timeline markers (`T+ms`)
- per-driver init durations
- console switch events logged
- optional key chord to toggle verbose boot overlay

---

## 9) Security and Integrity Considerations

- If secure boot enabled, treat boot assets as measured/signed content.
- Avoid parsing complex untrusted image codecs in kernel.
- Sanitize handoff framebuffer addresses and sizes.
- Restrict kernel cmdline overrides in locked production mode.
- Redact sensitive info from on-screen logs in release builds.

---

## 10) Performance and Memory Budgeting

Targets:

- zero-copy framebuffer writes where possible
- no large dynamic allocations in early boot
- bounded UART write latency
- splash draw under strict ms budget

Memory strategy:

- tiny static font (e.g., 8x16 bitmap)
- one optional double-buffer for flicker-sensitive platforms
- compressed assets in initrd, decompressed by user-space service

---

## 11) Suggested Component Breakdown (Concrete)

## In Bootloader

- `boot/console/serial_early.c`
- `boot/video/simplefb_setup.c`
- `boot/handoff/boot_io_info.h`

## In Kernel Core

- `core/kernel/console/console_manager.c`
- `core/kernel/console/log_buffer.c`
- `core/kernel/bootui/boot_splash.c`
- `core/kernel/bootui/boot_policy.c`

## In Drivers

- `core/drivers/uart/*`
- `core/drivers/video/simplefb/*`
- `core/drivers/gpu/*` (full modeset)

## In User-space Early Services

- `core/services/boot-uxd/`
- `core/services/log-forwarder/`
- `core/services/boot-handoffd/`

## In Product Stack

- `stack/branding/assets/`
- `stack/branding/policies/*.json`

---

## 12) MVP Roadmap

## Milestone 1 (Reliability First)

- UART early + normal console path.
- Kernel log ring buffer with replay.
- Framebuffer text sink.
- Static logo + kernel version in kernel boot UI.

## Milestone 2 (Adaptation)

- Board capability DB and runtime policy engine.
- Boot cmdline toggles and debug modes.
- Failover tests (display fail -> serial fallback).

## Milestone 3 (UX)

- user-space `boot-uxd` with animation/progress.
- clean handoff to compositor.
- secure asset validation.

## Milestone 4 (Optimization)

- performance profiling
- fast paths for common boards
- boot-time regression gating in CI

---

## 13) Test Strategy

- Unit tests for policy engine decision matrix.
- Driver probe/init tests (mocked MMIO).
- Integration boot tests on QEMU for x86_64, ARM64, RISC-V.
- Hardware-in-loop smoke tests per board family.
- Fault injection:
  - missing fb
  - broken logo asset
  - UART not responding
  - GUI service timeout
- Visual golden-image checks for splash layout.

---

## 14) Key Design Principles (Quick Reference)

1. **Serial first, always.**
2. **Kernel keeps minimal but robust visual fallback.**
3. **Rich UX belongs to user-space service.**
4. **Policy in kernel, branding in stack.**
5. **Board/arch capability-driven boot behavior.**
6. **Explicit fallback on every failure point.**

