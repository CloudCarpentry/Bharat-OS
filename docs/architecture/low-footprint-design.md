---
title: Low-Footprint Design and Analysis
status: Draft
owner: Documentation Working Group
last_updated: 2026-04-25
tags:
  - docs
  - architecture
see_also:
  - README.md
---
# Low-Footprint Design and Analysis

This document details the strategies and analysis for running Bharat-OS on resource-constrained hardware, specifically focusing on the **Nano** and **RT (Real-Time)** profiles.

## Overview
Bharat-OS is architected as a scaling microkernel. While its Tier 1 (64-bit) configurations support rich services like full VM and SMP, its **Tier 2 (EDGE32)** and **Nano** profiles are optimized for MCUs with limited RAM (128KB - 2MB) and Flash.

## Footprint Estimates (Tier 2 / Nano)

| Profile | RAM Constraint | Est. Kernel RAM | Est. Kernel Flash | Key Hardware |
| :--- | :--- | :--- | :--- | :--- |
| **Nano** | 128KB - 512KB | ~200KB - 250KB | <512KB | TI Hercules (Cortex-R4/5) |
| **RT / Edge** | 1MB - 2MB | ~350KB - 500KB | <1MB | TI Sitara AM2x (Cortex-R5F) |

## Strategy for Small Footprints

To achieve sub-megabyte footprints, the following architectural policies are applied:

### 1. Minimal Physical Memory Manager (PMM)
The PMM (`pmm.c`) uses a buddy allocator with minimal metadata. 
- **Overhead**: For a 2MB RAM system, the static PMM structures and `page_t` metadata total approximately **25KB**.
- **Optimization**: On "Nano" profiles, NUMA zones and complex cache-coloring lists are reduced or disabled to save static RAM.

### 2. Single-Processor (UP) Mode
By disabling Symmetric Multiprocessing (SMP):
- Per-core data structures are eliminated.
- Inter-processor Communication (IPI) and spinlock contention paths are removed.
- Cross-core URPC channels are not instantiated, saving significant buffer space.

### 3. MPU-only / MMU-lite Models
Instead of full demand-paging (Tier 1), small devices use simpler protection models:
- **MPU-only**: Direct physical addressing with region-based protection (e.g., standard for Hercules).
- **MMU-lite**: Static page table mappings established at boot, with no sparse VM/Page-fault overhead.

### 4. Subsystem Stripping
The following subsystems are explicitly removed or stubbed in the Nano/RT profiles:
- **Headless Mode**: Disables framebuffer drivers, boot GUIs, and widget toolkits.
- **Minimal VFS**: Replaces full filesystem support with a simple read-only storage or flat-file interface.
- **Embedded Network Stack**: Uses a specialized, low-overhead networking path (e.g., LwIP-style) instead of the full high-throughput stack.

## Case Study: TI Automotive MCUs

### TI Hercules (Safety Domain)
- **Constraint**: Up to 512KB Internal RAM.
- **Design**: The **Nano** profile fits by using MPU-only mode and static buffers. The kernel image remains under 256KB, leaving over 256KB for safety-critical user tasks.

### TI Sitara AM2x (Zonal Domain)
- **Constraint**: 2MB Internal SRAM.
- **Design**: The **RT/Edge** profile is the "sweet spot". It allows for a functional real-time kernel with deterministic IPC, while still fitting entirely within internal SRAM to avoid the latency of external memory.

## Conclusion
Bharat-OS demonstrates that a capability-based microkernel can scale down to automotive-grade MCUs without losing its core security and isolation benefits. Future work focuses on the "EDGE32 Tier 2" roadmap to further refine these 32-bit optimization paths.
