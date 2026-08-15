---
title: Hardware Capability Packs
status: Proposed
owner: Architecture Working Group
last_updated: 2026-08-12
tags:
  - architecture
  - hardware
  - delivery
  - profiles
see_also:
  - device-profiles-and-use-cases.md
  - ../adr/018-virtual-robotics-device-path.md
  - ../adr/ADR-012-can-subsystem-architecture.md
  - ../dev/hardware-feature-placement-rules.md
---

# Hardware Capability Packs

## Decision and terminology

Drone, automotive, TV/set-top-box, POS, and router are **hardware capability
packs**, not separate Bharat-OS versions and not new kernels. A pack is a
declarative delivery composition of drivers, UAPIs, libraries, services, stacks,
applications, and capability grants on the common Bharat-OS kernel spine.

A pack may be combined with an orthogonal device, personality, execution, memory
protection, and footprint profile. For example, the robotics pack may be used by
an RT MPU target or by a general-purpose MMU QEMU demonstration. Pack selection
must not imply that hardware exists: platform/HAL runtime discovery remains the
authority, and an unavailable required device makes activation fail closed.

This document is a roadmap and composition contract. It does not claim that the
listed production drivers or demos are implemented. A pack becomes selectable
only after its machine-readable manifest, component maturity declarations,
capability policy, target wiring, and validation evidence land together.

## Common pack contract

Each future manifest under `delivery/packs/` must declare:

- a stable pack identifier and manifest schema version;
- required, optional, and mutually exclusive components by repository-owned
  component identifier, never by a raw source-file list;
- required hardware classes and acceptable virtual backends;
- services and stacks to start, plus bounded dependency/startup ordering;
- the least-privilege capability grants for every application and service;
- supported MMU, MMU-Lite, and MPU modes, with unsupported combinations rejected;
- resource budgets, queue limits, deadlines, safe-state behavior, and critical
  service failure policy where applicable;
- maturity requirements and the tests/demos that constitute pack evidence.

The pack resolver belongs to delivery/build tooling. `sysmgr` consumes the
resolved, validated service graph; neither pack names nor product policy belong
in the kernel. Runtime hardware truth flows from platform discovery through HAL.
Drivers control hardware, services own arbitration and policy, libraries provide
client ergonomics, and stacks compose domain protocols.

Pack manifests may grant access only through named capability objects. They may
not embed pointers, MMIO addresses, IRQ numbers, raw syscall numbers, or mutable
cross-core objects. Failure to discover a required class, satisfy a dependency,
or issue a required grant aborts pack activation rather than silently degrading.

## Drone / robotics pack

### Priority and data path

The first robotics milestone is the generic sensor/actuator path accepted in
ADR 018, followed by physical bus backends. Its layering is:

```text
Application
    |
lib/sensor + lib/actuator
    |
sensormgr / actuator_mgr
    |
sensor/actuator UAPI
    |
drivers: I2C, SPI, PWM, GPIO
    |
HAL
    |
SoC/platform discovery
```

The physical-capability priority is:

1. GPIO, I2C, SPI, PWM, and UART;
2. CAN, ADC, timer/capture, and watchdog;
3. IMU and GPS sensor drivers;
4. motor actuator drivers and their safe-state integration.

The repository already has the intended placement seams in `core/services/device/`,
`core/drivers/sensor/`, `core/drivers/actuator/`, and `core/stacks/robotics/`.
Bus drivers expose mechanisms; `sensormgr` owns subscription/arbitration policy;
`actuator_mgr` owns arming, command validation, replay rejection, and safe output;
the robotics stack composes those generic interfaces without a drone-specific
kernel or UAPI.

### Required safety behavior

- Sensor samples use monotonic timestamps and bounded queues with explicit
  overflow reporting.
- Actuator commands require type-, rights-, scope-, generation-, and
  ownership-valid capabilities after fault-safe usercopy at an IPC boundary.
- Motor outputs remain disarmed and zeroed until an authorized arming transition.
- Watchdog expiry, required sensor loss, manager failure, or partial initialization
  invokes a documented service-owned safe-state policy.
- Managers never hold a device-table lock while awaiting remote completion, and
  physical backends preserve the MMU/MMU-Lite/MPU-neutral UAPI semantics.

The first demo remains the deterministic virtual robot data path. Physical I2C,
SPI, PWM, GPIO, IMU, and GPS implementations are subsequent evidence milestones,
not prerequisites for demonstrating the generic architecture.

## Automotive pack

### AUTO-P1-001: CAN framework first

CAN is the first automotive device/data-path milestone. The target composition is:

```text
core/drivers/class/can/ and core/drivers/devices/<controller>/
interface/include/bharat/uapi/device/can.h
interface/sdk/lib/device/can/
core/services/can/canmgr/
core/stacks/vehicle/
```

These names follow the repository's current layer layout; they supersede shorthand
such as `drivers/bus/can/`, `lib/device/can/`, or `services/network/can/` when that
shorthand would place controller mechanics or vehicle policy in the wrong layer.

The eventual public UAPI needs one canonical, versioned, fixed-width frame type.
Its payload remains FD-sized from the start even while P1 enables classical CAN:

```c
typedef struct bh_can_frame {
    uint32_t id;
    uint8_t len;
    uint8_t data[64];
    uint32_t flags;
    uint64_t timestamp_ns;
} bh_can_frame_t;
```

Before this becomes ABI, the contract change must define flag bits, identifier and
length validation, reserved/padding bytes, byte order, alignment, total size and
field-offset assertions, capability rights, and compatibility lock updates. The
existing internal `can_frame_t` definitions must be inventoried and converged;
this document does not create a parallel ABI authority.

P1 acceptance requires a bounded controller queue, explicit busy/backpressure,
hardware plus software acceptance filtering, monotonic receive timestamps,
bus-off/error reporting, Tx/Rx capability separation, and deterministic virtual
CAN tests. The demo connects a virtual ECU to Bharat-OS and a CAN monitor showing,
for example, engine RPM (`0x101`), vehicle speed (`0x102`), battery voltage
(`0x301`), and steering angle (`0x441`). Values are simulated and clearly labelled.

After the base device/data path is proven, add CAN-FD, UDS, DoIP, SOME/IP,
Automotive Ethernet, and TSN as separate drivers/services/stacks. AUTOSAR
compatibility is explicitly not a P1 objective.

## TV / set-top-box demo pack

The first TV pack demonstrates the display/input/audio architecture rather than
claiming a complete codec platform. Its minimum composition is:

- virtio-gpu/display and keyboard or virtual remote input;
- an audio device abstraction;
- framebuffer compositor and LVGL adapter;
- basic image viewer and simple monotonic media clock;
- Home, Settings, Network, Media Browser, and System Information applications.

A software decoder may exercise the media pipeline initially. H.264/H.265
hardware decode, HDMI, HDCP, DRM/content protection, audio DSP, and CEC remain
later milestones. Content-protection work must receive a separate security and
key-lifecycle review; it is not implied by enabling display or media components.

## POS device demo pack

The POS pack is a commercial demonstration composition built from display,
touch/input, network, serial, virtual barcode, virtual printer, and secure-storage
capabilities. Its application shows an item total and card, UPI, and cash actions,
with explicit network and printer state.

The virtual barcode and printer devices use the same capability-mediated device
interfaces intended for physical backends. The application receives no direct
MMIO, storage-key, or unrestricted network capability. Payment actions in the
initial demo are simulations: no production payment, PIN, cardholder-data, or
compliance claim is permitted until separate security contracts and evidence
exist. Secure storage grants should be scoped to application-owned objects, with
the security service retaining key policy.

This pack demonstrates GUI, networking, devices, secure capabilities, storage,
and the application SDK without adding POS policy to the kernel.

## Network-router demo pack

The initial virtual topology is:

```text
QEMU
 +-- virtio-net0 -> WAN
 `-- virtio-net1 -> LAN
```

The dashboard reports discovered interfaces, link state, receive/transmit packet
counters, routes, neighbours, and drops. Counters must come from the network
service data path rather than hard-coded presentation values. WAN and LAN device
capabilities are distinct; only the routing service may hold both, while the
dashboard receives read-only telemetry capabilities.

Phased functionality is DHCP, DNS, forwarding, ACL, NAT, then VLAN. Forwarding
must default off, and an absent or invalid policy must fail closed. Queue limits,
drop reasons, route ownership, counter saturation, and configuration rollback
need tests before the pack is considered more than a demo.

The result may be delivered as a **Bharat Router OS profile** for product naming,
but it remains a capability pack and service composition over the same kernel.

## Delivery sequence and evidence

1. Define and validate the generic pack manifest schema and fail-closed resolver.
2. Land the robotics virtual path as the reference pack, then physical bus/device
   backends in the stated priority order.
3. Complete AUTO-P1-001 by converging the internal CAN frame model before adding
   a public UAPI and SDK wrapper.
4. Compose the TV, POS, and router QEMU demos only from maturity-declared
   components, labelling virtual or simulated behavior in their UI and logs.
5. Add target-specific manifests only after each required architecture and memory
   protection mode has explicit support or an explicit unsupported result.

Every pack change requires focused service/driver tests, layer and CMake checks,
the applicable ABI check, all required target builds, and the all-architecture
QEMU gate. Pack marketing/status tables may advance only with recorded evidence.
