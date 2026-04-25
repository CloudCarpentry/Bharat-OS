---
title: USB Subsystem Roadmap
status: Draft
owner: Documentation Working Group
last_updated: 2026-04-25
tags:
  - docs
  - architecture
see_also:
  - README.md
---
# USB Subsystem Roadmap

## Purpose
This document defines the phased implementation plan for USB support in Bharat-OS. USB is treated as a bus subsystem, separating core logic from controllers and specific functional classes.

## USB Architecture Layers
1. **Layer 1: USB Core**: Descriptors, endpoints, interfaces, configurations, transfers, and enumeration state machines.
2. **Layer 2: Host Controllers**: Implementations for specific hardware (e.g., xHCI, EHCI).
3. **Layer 3: Class/Function Drivers**: Implementations for specific USB device profiles (e.g., HID, mass storage).
4. **Layer 4: Service Integration**: Handling device arrival, allow/deny policy, and routing via `devmgr`.

## Implementation Roadmap

### Phase U0: Documents and Baseline (Current)
- USB subsystem doc, core contracts, event model, build gating, devmgr hotplug contract.

### Phase U1: USB Host Core Baseline
- Core descriptors, device tree topology, enumeration state machine skeleton, hub awareness.

### Phase U2: xHCI Production Slice
- Real host controller integration, port detection, control transfers, descriptor reading.

### Phase U3: HID Support
- Keyboard, mouse, boot protocol, integration with the input subsystem.

### Phase U4: Mass Storage
- Block-device registration, attach/remove handling.

### Phase U5: Networking / Serial
- CDC ACM serial, limited network adapters.

### Phase U6: Gadget/Device Mode
- Reserved for future implementation once host mode matures.