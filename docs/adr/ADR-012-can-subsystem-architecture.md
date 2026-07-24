---
title: ADR-012: CAN Subsystem and Userspace Routing Architecture
status: Accepted
owner: Documentation Working Group
last_updated: 2026-04-25
tags:
  - docs
  - adr
see_also:
  - README.md
---
# ADR-012: CAN Subsystem and Userspace Routing Architecture

## Status
Accepted

## Context
Automotive, EV, and drone use-cases require a robust, deterministic, and isolated Controller Area Network (CAN) subsystem. Classical CAN (2.0A/B) and CAN FD both feature shared-medium arbitration and are prone to abuse or denial-of-service if arbitrary applications can directly manipulate the bus.

Additionally, in Bharat-OS's multikernel and capability-based architecture, embedding complex message routing, software filtering, and protocol parsing directly into the kernel ring-0 breaks our security and safety boundaries. We needed an architecture that scales from single-SoC Edge controllers to distributed, multi-ECU automotive systems.

## Decision
We have decided to split the CAN subsystem into two distinct layers:
1. **Generic Controller Core (Kernel/HAL):** A minimal layer (`core/drivers/can_core`) implementing basic interrupt handling, MMIO-safe controller access, timestamp capture, and fast-path ring enqueue/dequeue. The core defines a generic `can_controller_ops_t` vtable and an FD-ready `can_frame_t`.
2. **Userspace CAN Service (`core/services/can`):** A privileged userspace domain responsible for:
    - Client registration and IPC capability enforcement (Tx/Rx rights).
    - Software filtering and gateway routing between buses or remote ECUs.
    - Diagnostics, stats aggregation, and bus-off recovery policies.

### FD-Ready Frames
Even if classical CAN is the only hardware present, the internal `can_frame_t` utilizes a 64-byte payload and bits for BRS (Bit Rate Switch) to ensure forward compatibility with CAN FD without requiring future ABI breaks.

### Virtual CAN Backend
A `virt_can` backend was introduced alongside the core implementation to provide deterministic testing of routing policies, loopback modes, and FD framing on host systems without requiring physical hardware.

## Consequences
- **Security:** Errant or malicious user applications cannot directly crash the CAN bus or spoof critical messages. All operations are mediated by the CAN service and backed by IPC capabilities.
- **Reliability:** If the CAN routing logic panics, the `core/services/can` process can be restarted independently without crashing the entire system or interrupting the underlying hardware controller state machine.
- **Portability:** The kernel core provides a generic vtable. Adding an STM32 bxCAN or NXP FlexCAN driver only requires implementing the `can_controller_ops_t` and registering with the core.
- **Testing:** The architecture is fully testable natively on the host via `virt_can` unit tests.

## Notes
Hardware acceptance filters (when available) should be programmed dynamically by the CAN service by compiling client subscriptions. Software filtering acts as a strict secondary gateway. Higher-level protocols like UDS, J1939, and SOME-IP will be built as separate microservices layered on top of the CAN service, not baked into the base transport.