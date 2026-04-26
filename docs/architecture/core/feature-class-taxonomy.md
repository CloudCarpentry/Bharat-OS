---
title: Feature Class Taxonomy
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
# Feature Class Taxonomy

## Overview
The Feature Class Taxonomy abstracts low-level hardware capabilities into reusable, high-level behavioral classes. This taxonomy prevents the system from being tightly coupled to specific ISA extensions or vendor accelerators.

## Feature Classes

1. **Compute & Vector (`CLASS_COMPUTE_VECTOR`)**
   - Covers all SIMD, matrix multiplication, and vector processing units.
   - *Mapping:* NEON, SVE, AVX, RV-V.

2. **Cryptography (`CLASS_CRYPTO`)**
   - Covers hardware engines for hashing, symmetric, and asymmetric cryptography.
   - *Mapping:* AES-NI, ARMv8 Crypto, dedicated security co-processors.

3. **Tensor & Machine Learning (`CLASS_TENSOR_ML`)**
   - Covers neural processing units, tensor cores, and matrix accelerators.
   - *Mapping:* Edge TPUs, discrete NPUs.

4. **Packet Offload (`CLASS_PACKET_OFFLOAD`)**
   - Covers hardware acceleration for network checksums, TCP segment routing, and IPsec.
   - *Mapping:* SmartNICs, advanced MACs.

5. **Media & Display (`CLASS_MEDIA_DISPLAY`)**
   - Covers display controllers, video codecs, and 2D/3D graphics pipelines.
   - *Mapping:* GPUs, VPU codecs, display subsystems.

6. **Sensor & Actuator (`CLASS_SENSOR_ACTUATOR`)**
   - Covers real-time sensor ingestion, PWM controllers, and motion blocks.
   - *Mapping:* I2C/SPI sensor hubs, motor control loops.

7. **Storage Acceleration (`CLASS_STORAGE_ACCEL`)**
   - Covers DMA storage offload, NVMe acceleration.
   - *Mapping:* Dedicated flash controllers.

8. **Trust & Security (`CLASS_TRUST_SECURITY`)**
   - Covers secure enclaves, hardware keys, and isolation mechanisms.
   - *Mapping:* TrustZone, secure elements, root of trust.

9. **Timing & Real-Time (`CLASS_TIMING_RT`)**
   - Covers precision timing, TSN (Time-Sensitive Networking), and deterministic execution limits.

10. **Power & Thermal (`CLASS_POWER_THERMAL`)**
    - Covers dynamic voltage and frequency scaling, sleep state controllers, and thermal sensors.

## Profile Examples

- **Tiny Device (IoT Sensor):**
  - Uses `CLASS_SENSOR_ACTUATOR` and `CLASS_POWER_THERMAL`.
- **Mobile / Wearable:**
  - Employs `CLASS_MEDIA_DISPLAY`, `CLASS_CRYPTO`, `CLASS_POWER_THERMAL`.
- **Desktop:**
  - Broadly leverages `CLASS_COMPUTE_VECTOR`, `CLASS_MEDIA_DISPLAY`, `CLASS_STORAGE_ACCEL`.
- **Datacenter / Edge Server:**
  - Heavy reliance on `CLASS_TENSOR_ML`, `CLASS_PACKET_OFFLOAD`, `CLASS_COMPUTE_VECTOR`.
- **Robot / Drone:**
  - Real-time focus with `CLASS_TIMING_RT`, `CLASS_SENSOR_ACTUATOR`, `CLASS_TENSOR_ML` (for vision).
