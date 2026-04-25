---
title: Driver Model Baseline Architecture
status: Draft
owner: Documentation Working Group
last_updated: 2026-04-25
tags:
  - docs
  - architecture
see_also:
  - README.md
---
# Driver Model Baseline Architecture

## Purpose
This document defines the architecture for the Bharat-OS driver baseline. It outlines the core models for devices, buses, and driver registration, and clearly separates what belongs in the kernel versus what belongs in drivers and services.

## Device Classes
A unified vocabulary for devices allows drivers and services to communicate effectively. Common classes include:
- GPIO, PINCTRL, PWM
- I2C, SPI (Controllers and Devices)
- USB (Host, Device, Interface)
- LED, Input, Display, Sensor
- Watchdog, RTC, Power, Net, Storage

## Registration Rules
- **Mechanism vs. Policy**: The kernel strictly provides the mechanism (memory mapping, IRQ routing). Drivers handle device control. Services (like `devmgr`) handle policy (orchestration, permission, hotplug).
- Drivers must register with the centralized driver registry using standardized driver descriptors.
- Devices are discovered via bus enumeration or static platform configuration and registered with the device registry.

## Lifecycle States
- **Match**: A device's compatibility IDs are matched against registered drivers.
- **Probe**: Initial hardware initialization and resource allocation. Must explicitly succeed or cleanly fail.
- **Remove**: Teardown and cleanup. No state must leak after removal.
- **Suspend/Resume**: Stubbed or active power state transitions.
- **Fault/Inactive**: Explicit state tracking if the device or driver errors out.

## Error and Fault Rules
- No silent failures during `probe()`.
- Error codes must be structured and propagated cleanly.
- Fault paths must explicitly transition the device to a `FAULT` state and notify `devmgr`.

## Suspend/Resume Contract
Drivers must provide `suspend` and `resume` hooks (even if stubbed initially). On resume, drivers should establish a safe default state before user-space interactions resume.

## Hotplug Event Contract
Events emitted by the driver core (e.g., `ADDED`, `REMOVED`, `CHANGED`, `FAULT`) must be propagated to `devmgr` to handle dynamic device lifecycles.

## Build and Profile Gating
Driver sub-systems must be isolated using CMake flags (e.g., `BHARAT_ENABLE_GPIO`, `BHARAT_ENABLE_USB`). This prevents bloat in tiny edge profiles while enabling rich features on full profiles.