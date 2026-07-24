# Accelerator Manager (accelmgr)

## Overview
The Accelerator Manager (`accelmgr`) is a core user-space service responsible for policy orchestration over advanced hardware features (NPUs, GPUs, and other compute offloads). It enforces permissions, admissions, queues, and thermal constraints.

## Placement & Boundaries
- **Policy Engine:** Resides entirely in `services/device/accelmgr/`.
- **Hardware Agnostic:** Uses the Feature Class Taxonomy (e.g. `CLASS_TENSOR_ML`). It contains NO vendor-specific driver code.
- **Dispatch Tie-in:** Provides the telemetry and power state (`backend_dispatch_context_t`) that `lib/runtime/` uses to select between software and hardware backends.

## Responsibilities
1. **Backend Admission:** Controls which processes can utilize advanced hardware.
2. **Queue Policy:** Determines prioritization when hardware is contended.
3. **Thermal Throttling:** Interfaces with `powermgr` and `thermalmgr` to degrade accelerators (or force software fallbacks) when temperatures rise.
4. **Safe-mode Disablement:** Hooks into system health and fault domains to entirely disable failing accelerators.
5. **Telemetry Surfacing:** Collects metrics (backend selected, fallbacks, throttles, faults) via `accel_telemetry.idl`.
