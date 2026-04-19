# Hardware Capability Model

## Overview
The Hardware Capability Model provides a normalized, architecture-agnostic way to describe hardware features discovered at boot. It establishes a unified contract for CPU features, memory/protection, SoC capabilities, and their respective states (absent, present, optional, required, degraded).

## Principles
- **No Raw ISA Names:** Components must communicate via generalized capability flags, not architecture-specific macros (e.g., use `CAP_CPU_VECTOR` instead of checking for NEON or AVX).
- **Single Source of Truth:** `bharat_hw_caps_t` is the canonical record populated during boot.
- **Strict Boundary:** No board-specific or platform-specific structures are exposed outside the `platform/` directory.

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

## Flow
1. **Boot/Discovery:** `platform/` code queries the hardware/device tree.
2. **Translation:** `platform/` maps raw discoveries into `bharat_hw_caps_t`.
3. **Registration:** The populated capability structure is registered with the Kernel Capability Registry.
4. **Consumption:** `services/` and `lib/runtime` query the registry to make dispatch and policy decisions.
