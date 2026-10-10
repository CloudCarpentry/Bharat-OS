---
title: Integer progress bars during boot framebuffer rendering
status: Accepted
owner: Kernel and Services Working Group
last_updated: 2026-10-10
---

# ADR-037: Integer boot UI progress

The RISC-V64 graphical dashboard runs during kernel bootstrap before a thread
owns floating-point state. Calling its fractional progress-bar API generated
`fmv.w.x` with supervisor FS disabled, causing an illegal-instruction exception
at the dashboard's first progress bar.

Add an integer percentage constructor for this boot UI path. Clamp values to
0–100 and calculate fill widths with bounded integer arithmetic. The dashboard
uses that constructor; its drawing path does not access floating-point values.
The existing fractional constructor and its rendering behavior remain available
to userspace callers. Private widget data selects the appropriate representation.

This is an additive source-library API, with no syscall, wire structure, service
readiness, capability, scheduler, or floating-point ownership change. It does
not qualify other kernel floating-point users or make GUI boot failure healthy.
The host regression checks clamping, empty/full/fractional fills, the existing
fractional API and pool exhaustion; RISC-V QEMU verifies the boot rendering path.
