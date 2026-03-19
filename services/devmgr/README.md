# services/devmgr

## Purpose
Central device enumeration and lifecycle service. The authority on attached hardware devices, buses, and firmware.

## Responsibilities
- **Bus Tree Enumeration**: Owns the PCIe/CXL tree and hierarchical topology.
- **Hotplug Support**: Detects and manages device insertion and removal.
- **Driver Binding**: Maps devices to their corresponding driver binaries or services.
- **Device Reset/Recovery**: Orchestrates resetting misbehaving hardware.
- **Firmware Coordination**: Loading or directing firmware for initial device states.
- **Isolation Policy**: Assigning IOMMU groups, VFIO/SR-IOV domains.

## Dependencies
- **May depend on:** `lib/runtime`, `lib/ipc`, standard C library headers.
- **Must not depend on:** `subsys/*`, `ui/*`, direct kernel-private headers.

## Immediate TODOs
- Implement root PCIe complex enumeration stub.
- Register endpoints with `servicemgr` / `namesvc`.

## Status
Stub.
