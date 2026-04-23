# Bharat-OS Footprint Tier Model

This document defines the **official footprint contract** for Bharat-OS targets.

## Tier Definitions

| Tier | Name | RAM | Flash | Purpose |
|---|---|---:|---:|---|
| S0 | Micro | 128–512 KB | 512 KB–2 MB | Bring-up / MPU-only deployments |
| S1 | Small Embedded | 512 KB–2 MB | 2–8 MB | Real embedded systems |
| S2 | Edge | 2–16 MB | 8–64 MB | Rich services and gateway-class systems |
| S3 | Personality | 32 MB+ | 64 MB+ | Linux/Android personality hosting |

## Contract Semantics

- **boot_min** values are hard admission limits. Targets below this are rejected.
- **usable_min** values are policy limits for stable operation.
- **recommended** values are planning guidance and CI warning thresholds.

## Architecture Principle

The footprint model is intentionally independent from ISA and personality:

- Kernel provides mechanism.
- Services provide policy.
- Profiles provide composition.
- ISA remains an implementation detail.

## Machine-readable Source of Truth

The machine-readable matrix lives at:

- `configs/footprint/footprint_matrix.csv`

Build, package, and run tooling must resolve per-target constraints from that matrix.
