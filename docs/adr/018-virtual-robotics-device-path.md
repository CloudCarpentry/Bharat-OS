# ADR 018: Generic Virtual Robotics Device Path

- **Status:** Accepted
- **Date:** 2026-08-12

## Context

Bharat-OS needs a demonstrable robotics data path before physical board support is
available.  A drone-specific API would couple applications to one product and
would make later ground-robot and industrial use cases need parallel interfaces.

## Decision

The public boundary is two generic, fixed-width device UAPIs:
`bharat/uapi/device/sensor.h` and `bharat/uapi/device/actuator.h`.  Sensor samples
carry an explicit sensor identifier, type, monotonic timestamp, flags, value
count, and four scalar values.  Actuator commands are versioned and carry a
monotonic request identifier so a backend can reject replay.

The QEMU robot platform owns one virtual-device instance containing an IMU, GPS,
temperature sensor, battery sensor, distance sensor, and four motors.  It passes
those device contexts to `sensormgr` and `actuator_mgr`; neither service nor
driver introduces shared mutable global state.  A service instance and its
caller are responsible for serializing access to its bounded tables.

Sensor streams use a slot plus generation handle.  Subscription tables are
bounded and report `BH_ERR_BUFFER_FULL`; a closed or recycled handle reports
`BH_ERR_STALE_CAPABILITY`.  Motor outputs are denied until armed, normalized
commands are range checked, and non-increasing request identifiers are rejected.
Initialization or sampling failure puts all motors into the disarmed zero-output
safe state.

The robotics stack composes these mechanisms and renders the terminal dashboard.
It remains application/service policy and does not add robotics policy to the
kernel.  The implementation is backend-neutral C and has the same semantics on
MMU, MMU-Lite, and MPU profiles.

## Security and capability boundary

The manager-issued sensor stream and actuator handles are opaque, fixed-width
values rather than pointers.  `actuator_mgr` is the authority boundary for motor
instances and denies an instance when its grant is absent.  This initial virtual
transport is in-process; an eventual IPC binding must validate capability type,
rights, scope, generation, and ownership before invoking these manager methods.

## Consequences

- QEMU/host demos can exercise a complete generic sensor and actuator path.
- Queue capacity, stale handles, replay, ranges, and safe-state behavior are
  deterministic and testable.
- The virtual waveforms are deterministic demonstrations, not physical models.
- Production IPC and physical I2C/SPI/PWM backends remain follow-up work and do
  not change this UAPI.
