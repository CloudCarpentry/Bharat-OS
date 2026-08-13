---
title: Robotics Use Case
status: Draft
owner: Team
last_updated: '2024-01-01'
tags:
- doc
see_also:
- none
---
# Robotics Use Case

## Flow

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

## Problem
In a traditional approach, raw data ingested by the camera might have to be written into a CPU-owned buffer, evaluated by the CPU for navigation, then copied into NPU-accessible memory for vision models, and then copied to GPU memory for visualization or SLAM mappings.

## Solution
HMEM provides a single sharing/mapping abstraction. The frame is placed into HMEM and given mapped capabilities to the CPU, Vision NPU, and GPU visualization without redundant memory copies.

This matters heavily for real-time and embedded contexts such as:
```text
drone perception
factory robot vision
autonomous navigation
SLAM
sensor fusion
object detection
```