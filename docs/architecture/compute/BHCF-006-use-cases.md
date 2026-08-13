---
title: 'BHCF-006: Use Cases'
status: Draft
owner: Team
last_updated: '2024-01-01'
tags:
- doc
see_also:
- none
---
# BHCF-006: Use Cases

## Purpose
This document outlines technical and commercial use cases for the Bharat Heterogeneous Compute Fabric (BHCF) and its core memory mechanism, HMEM. These use cases show that HMEM is not solely an AI feature but a generalized heterogeneous memory component.

## Summary Matrix

| Product       | Example HMEM use                |
| ------------- | ------------------------------- |
| Drone         | Camera → NPU → navigation       |
| Automobile    | Camera/radar/lidar → AI         |
| Robot         | Vision/sensor buffers           |
| TV            | Decoder → AI upscaler → display |
| Set-top box   | Video/DRM/display buffers       |
| Mobile/tablet | Camera → ISP → NPU/GPU          |
| Router        | NIC/DPU packet buffers          |
| POS           | Camera/scanner/AI inference     |
| Factory       | Machine vision/PLC gateway      |
| Appliance     | Sensor/DSP/control pipelines    |
| AI edge box   | Tensor/model/KV memory          |
| Desktop       | GPU/AI/media workloads          |

## Specific Use Cases

### 1. Robotics Use Case
*(See [Robotics Use Case Deep Dive](use-cases/robotics.md))*

```text
Camera
   │
   ▼
HMEM frame
   │
   ├─────────► Vision NPU
   │
   ├─────────► GPU visualization
   │
   └─────────► CPU navigation
```
This avoids requiring multiple stage buffers, providing one mapping abstraction for perception, navigation, object detection, etc.

### 2. Automotive Use Case

```text
Camera/Radar/Lidar
        │
        ▼
      HMEM
        │
 ┌──────┼─────────┐
 ▼      ▼         ▼
ISP    NPU       CPU
 │      │         │
 └──────┼─────────┘
        ▼
 Decision/control
```
Potential benefits include:
```text
reduced copying
bounded ownership
capability isolation
better accelerator interoperability
lower memory bandwidth pressure
```
*(Note: HMEM does not inherently claim or provide safety certification.)*

### 3. Media/TV/Set-Top-Box Use Case

```text
Decoder
   │
   ▼
Video-frame HMEM
   │
   ├──── GPU compositor
   │
   ├──── AI upscaler
   │
   └──── display engine
```
This avoids a pipeline where the frame must be copied to an AI buffer, then copied to a GPU buffer, then copied to the display buffer.

### 4. DPU/Networking Use Case

```text
NIC
 │
 ▼
DPU
 │
 ▼
HMEM packet/buffer
 │
 ├──── CPU network service
 │
 └──── crypto accelerator
```
Future possibilities:
```text
zero-copy packet processing
crypto without CPU bounce buffers
shared DMA-visible memory
capability-contained packet sharing
```

### 5. Storage/DMA Use Case

This use case strongly proves HMEM is not an AI-only feature. It supports:
```text
NVMe
storage DMA
compression
checksum
encryption
```

Example:
```text
NVMe
 │ DMA
 ▼
HMEM
 │
 ├── crypto
 ├── decompression
 └── application
```

### 6. GPU Graphics Use Case

Eventual use includes:
```text
textures
framebuffers
vertex buffers
compute buffers
camera/display surfaces
```
*(Note: HMEM is not a graphics API. The Graphics SDK/runtime remains above it.)*
