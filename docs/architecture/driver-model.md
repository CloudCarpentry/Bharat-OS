# Driver Model

## Overview

Traditional monolithic kernels execute hardware drivers in Ring-0. A single bug or buffer overflow in a driver can panic the entire operating system.

In Bharat-OS, **all device drivers execute strictly in unprivileged user-space (Ring-3).**

## Architecture

- **Hardware Isolation**: Drivers are standard applications running in isolated tasks. They are granted specific capabilities to map MMIO (Memory-Mapped I/O) regions and bind to specific IRQ (Interrupt Request) objects.
- **VMM/IOMMU Enforcement**: The IOMMU (Intel VT-d, AMD-Vi, ARM SMMU) is strictly configured by the microkernel to prevent malicious or malfunctioning devices from performing DMA (Direct Memory Access) attacks on arbitrary physical memory.
- **IPC Wrapping**: Applications do not talk to hardware. They talk to the Driver Task via fast Synchronous IPC or URPC ring buffers.
- **Recovery**: If a network driver crashes due to a vulnerability, the microkernel merely tears down the isolated task space and transparently restarts the driver. The core OS remains entirely unaffected.

---


## High-Performance Service Patterns

### Message-based USB stack (plug-and-play safe)

In line with ADR-003 multikernel messaging, USB is modeled as a distributed user-space service:

- A root USB bus manager owns host-controller capabilities.
- Each enumerated USB device is bound to a dedicated micro-driver thread/task.
- Driver tasks communicate with the bus manager and clients over capability IPC/URPC queues.
- A malfunctioning USB function driver can be restarted without taking down the entire I/O subsystem.

### XDP-like network fast-path

For high-throughput AI and storage traffic, NIC drivers may expose an early packet fast-path:

- Stateless filters/classifiers execute at RX ingress before full protocol-stack handoff.
- Eligible flows can be steered into zero-copy rings or direct service endpoints.
- Non-matching traffic always falls back to the regular user-space network stack.

This provides line-rate packet handling for simple routes without weakening isolation.

### Automated HAL matching via capability tree

Plug-and-play device bring-up is metadata-driven:

- Device IDs (PCI/USB/ACPI/DT) are matched against a signed capability policy tree.
- The kernel grants only the minimal capabilities required to the selected driver service.
- Optional blocks (NPU, specialty USB controller, secondary NIC) remain inaccessible until matched and explicitly delegated.

This keeps enumeration deterministic and least-privilege by default.
