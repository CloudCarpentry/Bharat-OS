---
title: Capability-mediated GUI input stream
status: Accepted
date: 2026-08-13
---

# Context

The GUI showcase previously satisfied the LVGL input symbol with an application
stub that always returned zero events. Linking the device input manager into the
application would make the demonstration interactive, but would collapse the
driver, service, and application trust boundaries.

# Decision

`inputmgr` is the owner of the normalized, bounded input queue. GUI clients find
its version-one endpoint through nameservice and invoke a bounded drain operation.
The endpoint capability is the authority to read input; clients receive fixed-
width events by value and never receive device or queue pointers. A drain returns
at most 16 events. Reserved fields are zero, response length and count are checked,
and any lookup, version, transport, status, or payload error fails closed and
invalidates the cached endpoint so a restarted service can be rediscovered.

LVGL owns cursor and key state on its UI event-loop thread. It accepts only known
relative axes, the primary button, and supported navigation keys. Relative-axis
addition saturates before coordinates are clamped to the queried display.

# Consequences

The application cannot bypass `inputmgr` to read VirtIO queues or QEMU devices.
The QEMU visual gate may inject host input only through QMP, must observe guest
acceptance, and must prove a bounded before/after pixel change. Service publication
and VirtIO-to-service routing remain required before the end-to-end gate can pass;
there is intentionally no in-process fallback while that runtime work is pending.

This contract is backend-neutral and does not change MMU, MMU-Lite, MPU, syscall,
or cross-core ownership behavior.
