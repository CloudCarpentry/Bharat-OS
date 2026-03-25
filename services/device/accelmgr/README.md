# services/device/accelmgr

## Purpose
This module is a **device policy manager**, not a driver. It owns routing policy, fallback logic, and model selection. Real accelerator hardware control drivers reside in `drivers/accel/`.

Hardware-aware accelerator abstraction service. Critical for future hardware architectures involving offloaded computing elements.

## Responsibilities
- **Hardware Abstraction**: Handles GPUs, NPUs, FPGAs, and DSPs.
- **Command-Queue Brokering**: Manages issuing commands to accelerator queues.
- **Memory Registration**: Facilitates shared memory setup for hardware offloading.
- **Capability Issuance**: Grants tasks rights to utilize specific accelerators.
- **FPGA Bitstream Deployment**: Loads and dynamically configures FPGAs.
- **Backend Selection**: Runtime routing of tasks to the most suitable backend.
- **Throttling & Performance Counters**: Collects metrics and throttles workloads if necessary.

*(Note: The kernel only provides secure queue doorbells, DMA isolation, interrupt delivery, and shared memory registration.)*

## Dependencies
- **May depend on:** `lib/runtime`, `lib/ipc`, standard C library headers.
- **Must not depend on:** `subsys/*`, `ui/*`, direct kernel-private headers.

## Immediate TODOs
- Register capability API to allocate queues.
- Hook up basic mock DSP/GPU abstractions.

## Status
Stub.
