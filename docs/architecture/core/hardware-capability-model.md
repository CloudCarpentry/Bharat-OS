---
title: Hardware Capability Model
status: Proposed
owner: Documentation Working Group
last_updated: 2026-04-25
tags:
  - docs
  - architecture
  - core
see_also:
  - README.md
---
# Hardware Capability Model

## Overview
The Hardware Capability Model provides a normalized, architecture-agnostic way to describe hardware features discovered at boot. It establishes a unified contract for CPU features, memory/protection, SoC capabilities, and their respective states (absent, present, optional, required, degraded).

## Principles
- **No Raw ISA Names:** Components must communicate via generalized capability flags, not architecture-specific macros (e.g., use `CAP_CPU_VECTOR` instead of checking for NEON or AVX).
- **Single Source of Truth:** `bharat_hw_caps_t` is the canonical record populated during boot.
- **Strict Boundary:** No board-specific or platform-specific structures are exposed outside the `core/platform/` directory.

## Capability Categories

### CPU Features
- Atomics (e.g., CAS, AMO)
- Vector/SIMD (e.g., NEON, AVX, RV-V)
- Cryptography (e.g., AES-NI, ARMv8 Crypto)
- Virtualization (e.g., VMX, SVM, EL2)
- Timers and Performance Monitors
- Cache Management and Coherency

### Memory & Protection
- MPU (Memory Protection Unit)
- MMU-lite (Basic Virtualization)
- MMU (Full Virtual Memory)
- IOMMU/SMMU
- DMA Coherency

### SoC & Peripherals
- DMA Controllers
- Watchdogs
- Accelerators: GPU, NPU, ISP, DSP, Video Codec
- Advanced Networking: TSN, CAN
- Radios (Wi-Fi, Bluetooth)
- Secure Enclaves (e.g., TrustZone, SGX)

## State Representation
Capabilities are not merely binary. Their state is captured as:
- `ABSENT`: Hardware does not support the feature.
- `PRESENT`: Hardware supports the feature and it is available.
- `OPTIONAL`: Feature may be enabled by policy.
- `REQUIRED`: System cannot boot or function properly without it.
- `DEGRADED`: Hardware is present but failing, thermally throttled, or partially isolated.

## Implemented authority and flow
Architecture backend support, CPU runtime facts, and platform runtime facts are intentionally different authorities. CPU facts are aggregated as local, system-wide intersection (`SYSTEM_ALL`), and system-wide union (`SYSTEM_ANY`). Platform facts such as IOMMU presence and DMA coherency require platform evidence and are false when unknown.

The effective `hal_hw_caps_t` snapshot progresses monotonically through raw discovery, CPU finalization, and freeze. The primitive registry may normalize only the frozen snapshot. See [ADR-021](../../adr/ADR-021-hardware-capability-freeze-authority.md) for the lifecycle, failure behavior, and current boot order.
