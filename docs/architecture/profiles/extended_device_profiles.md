---
title: Extended Device Profiles and Use Cases Matrix
status: Draft
owner: Documentation Working Group
last_updated: 2026-04-25
tags:
  - docs
  - architecture
  - profiles
see_also:
  - README.md
---
# Extended Device Profiles and Use Cases Matrix

This document defines the comprehensive matrix of target deployment profiles for Bharat-OS, spanning edge microcontrollers to data center servers. It details the required memory models, scheduler mixes, hardware subsystems (BUS, SoC, DMA, Accelerators), and the future roadmap for distributed kernel connectivity.

## Core Architectural Scaling Principles

Bharat-OS utilizes a unified microkernel spine that scales up and down through strict capability profiles. Rather than relying on unstructured `#ifdef` directives, the system enforces boundary definitions via `kernel_profile_t` and CMake build flags.

The three foundational kernel execution tiers map as follows:
- **RT (Real-Time):** Deterministic bounds, strict preemption, static memory pools, MPU/MMU-Lite.
- **GP (General Purpose):** High-throughput fairness, dynamic paging, MMU-Full.
- **MIX (Mixed Criticality):** Partitioned spatial/temporal isolation (e.g., RT domains isolated from GP domains via uRPC bridges).

## Comprehensive Profile Definitions

### 1. Desktop / Workstation
- **Description:** High-performance, user-interactive environments supporting rich GUIs and POSIX/Linux personality layers.
- **Memory Model:** `MMU_FULL` (Demand paging, COW, large pages).
- **Scheduler:** `GP` (General Purpose fair scheduling, multi-core SMP).
- **Hardware Integration:**
  - PCIe, USB 3.0+, NVMe storage, robust Display/Compositor pipelines.
  - High-performance DMA and IOMMU isolation.
  - GPU acceleration support via device driver framework.
- **Personality:** Linux-like userland, advanced shell and UI stack.

### 2. Mobile / Tablet
- **Description:** Power-constrained, interactive devices demanding responsive UI and secure application sandboxing.
- **Memory Model:** `MMU_FULL` with aggressive reclaim/COW.
- **Scheduler:** `MIX` (Foreground responsiveness paired with background power-saving limits; AI Governor power profiling).
- **Hardware Integration:**
  - ARM/RISC-V SoCs, DSI/eDP displays, MIPI CSI cameras.
  - Low-power DDR, suspend/resume deep sleep states.
- **Personality:** Android-like / Mobile-Linux environment (Binder-like IPC hooks, ashmem).

### 3. TV / Set-Top Box
- **Description:** Media-heavy streaming and display appliances.
- **Memory Model:** `MMU_FULL` with large contiguous pools for media decode.
- **Scheduler:** `GP` or `MIX` (Ensuring zero frame-drop priority for decode pipelines).
- **Hardware Integration:**
  - HDMI out, hardware video decode accelerators, DSPs.
  - Secure boot and DRM/TrustZone integration.

### 4. Home Appliance (White Goods)
- **Description:** Simple interface appliances (e.g., washing machines, basic refrigerators).
- **Memory Model:** `MMU_LITE` or `MPU_ONLY`.
- **Scheduler:** `RT` (Fixed priority for motor/sensor control).
- **Hardware Integration:**
  - Low-end ARM Cortex-M or R, RISC-V microcontrollers.
  - I2C, SPI, UART, PWM.
  - Small static memory footprint (sub 1MB RAM).

### 5. Smart Home Appliance
- **Description:** Network-connected home devices with richer UIs (e.g., smart thermostats, smart displays).
- **Memory Model:** `MMU_LITE` or `MMU_FULL` (depending on display needs).
- **Scheduler:** `MIX` (RT for physical control, GP for networking/UI).
- **Hardware Integration:**
  - WiFi/BLE radios, simple framebuffers or 2D accelerators.
  - OTA update support via A/B partition.

### 6. IoT Device
- **Description:** Ultra-constrained sensor nodes and edge endpoints.
- **Memory Model:** `MPU_ONLY`.
- **Scheduler:** `RT` (Strictly deterministic, event-driven, tickless idle).
- **Hardware Integration:**
  - Battery-powered SoCs, minimal peripheral set (I2C/SPI), no display.
  - Heavy reliance on low-power idle and fast wakeup.

### 7. IIoT (Industrial IoT) Device
- **Description:** Ruggedized, high-reliability industrial sensors and gateways.
- **Memory Model:** `MPU_ONLY` or `MMU_LITE`.
- **Scheduler:** `RT` (Hard real-time guarantees, bounded jitter).
- **Hardware Integration:**
  - Industrial buses (Modbus, CAN, PROFINET).
  - Watchdog integration, synchronous fault containment domains.
  - Secure boot and strict hardware capability bounds.

### 8. Data Center / Server
- **Description:** High-density compute nodes requiring massive scalability and throughput.
- **Memory Model:** `MMU_FULL` with NUMA awareness.
- **Scheduler:** `GP` (Highly scalable, NUMA-aware load balancing).
- **Hardware Integration:**
  - Multi-socket SMP, high-speed Ethernet (100G+), NVMe-oF.
  - SR-IOV, IOMMU, virtualization support (VirtIO).
  - AI accelerators / GPUs via PCIe pass-through.

### 9. Automobile (ECU / Infotainment)
- **Description:** Distributed vehicle architectures spanning critical safety domains to rich infotainment.
- **Memory Model:**
  - ECU: `MPU_ONLY` or `MMU_LITE`.
  - Infotainment: `MMU_FULL`.
- **Scheduler:** `MIX` (Hard RT for braking/powertrain, GP for media/navigation).
- **Hardware Integration:**
  - Automotive SoCs (e.g., Cortex-R for safety islands, Cortex-A for IVI).
  - CAN, CAN-FD, LIN, Automotive Ethernet (TSN, SOME/IP).
  - Strong capability isolation between domains.

### 10. Drone / UAV
- **Description:** Flight controllers and computer vision payloads.
- **Memory Model:** `MMU_LITE` or `MIXED` (depending on compute payload).
- **Scheduler:** `MIX` (Hard RT for flight dynamics, GP for telemetry/video).
- **Hardware Integration:**
  - High-speed SPI for IMUs/sensors, PWM for ESCs.
  - Image Signal Processors (ISP), hardware timers.
  - Watchdog-safe recovery pathways.

### 11. Robotics
- **Description:** Autonomous ground or industrial robots requiring complex sensor fusion and actuation.
- **Memory Model:** `MMU_FULL` (for ROS2-like stacks and AI) with `MPU` safety islands.
- **Scheduler:** `MIX` (Distributed IPC between AI path planning and RT motor control).
- **Hardware Integration:**
  - LiDAR/Camera interfaces (MIPI), ROS2 hardware acceleration.
  - Dedicated AI coprocessors/NPU integration.

## Distributed Kernel Roadmap

A futuristic goal of Bharat-OS is the **Distributed Multi-Kernel** extension. This architecture allows connected devices (e.g., an IIoT mesh or distributed Automotive ECUs) to operate under a unified OS fabric.

### How it works:
1. **Unified IPC/uRPC Plane:** The same capability-governed uRPC used for local cross-core communication is extended over network boundaries (e.g., Ethernet/TSN).
2. **Location Transparency:** A service on an Edge Gateway can invoke a capability on an IIoT node exactly as if it were a local endpoint.
3. **Transport Adapters:** The kernel introduces transport adapters that serialize local uRPC messages onto standard network protocols, verified by crypto-signed capability envelopes.
4. **Fault Awareness:** Unlike traditional transparent distributed systems, Bharat-OS makes latency and partition faults explicit. The AI Governor and scheduler manage dynamic placement based on node health and network jitter.

By extending the multi-kernel messaging model across the network, Bharat-OS bridges the gap between single-machine RTOS guarantees and cloud-scale orchestration.
