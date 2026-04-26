# Bharat-OS Driver Registry Contract (D0 Baseline)

This document describes the hardened contract for driver registration and device-to-driver binding in Bharat-OS.

## Canonical Descriptors

The following structures are established as the canonical descriptors in `core/drivers/include/driver_core.h`:

- `device_desc_t`: Describes device identity and hardware properties.
- `driver_desc_t`: Describes driver's capabilities and lifecycle operations.
- `device_binding_t`: Tracks the active relationship and lifecycle state between a device and a driver.

## Registry and Identity

Drivers and devices are registered via `driver_register()` and `device_register()`.
The registry assigns unique dynamic IDs:
- `driver_registry_id`
- `device_registry_id`

Name uniqueness is enforced at registration time.

## Deterministic Matching

Device-to-driver matching follows a strict deterministic scoring model:

1. **Match Score**: Calculated based on compatible strings, vendor/device IDs, and device classes.
2. **Explicit Priority**: If match scores are tied, the driver with the higher `priority` value is selected.
3. **Ambiguity Rejection**: If both match score and priority are equal, the match is rejected as ambiguous (returns `-SYS_EBUSY`).

## Lifecycle State Machine

The `device_binding_t` object tracks the lifecycle of a bound device/driver pair. Transitions are enforced by the core:

- `DRIVER_STATE_REGISTERED`: Initial state (for drivers).
- `DRIVER_STATE_MATCHED`: A driver has been successfully matched to the device.
- `DRIVER_STATE_PROBED`: The driver's `probe()` function has succeeded.
- `DRIVER_STATE_STARTED`: The device is active and running.
- `DRIVER_STATE_STOPPED`: The device is suspended or stopped but remains bound.
- `DRIVER_STATE_REMOVED`: The binding has been terminated and the slot cleared.
- `DRIVER_STATE_FAILED`: An operation (like `probe`) failed; allows for retries.

### Transition Rules

- `MATCHED -> PROBED` or `FAILED -> PROBED` via `device_binding_probe()`.
- `PROBED -> STARTED` or `STOPPED -> STARTED` via `device_binding_start()`.
- `STARTED -> STOPPED` via `device_binding_stop()`.
- Any (except `REMOVED`) `-> REMOVED` via `device_binding_remove()`.

When a binding is removed, its slot in the registry is freed for reuse.

## Future Capability Mediation

D0 does not wire active capability checks. Later phases should require explicit capabilities for:
- Driver registration.
- Device binding and access.
- DMA mapping and IRQ routing.
- Lifecycle operations.
