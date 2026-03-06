# Driver Model

## Overview

Traditional monolithic kernels execute hardware drivers in Ring-0. A single bug or buffer overflow in a driver can panic the entire operating system.

In Bharat-OS, **all device drivers execute strictly in unprivileged user-space (Ring-3).**

## Architecture

- **Hardware Isolation**: Drivers are standard applications running in isolated tasks. They are granted specific capabilities to map MMIO (Memory-Mapped I/O) regions and bind to specific IRQ (Interrupt Request) objects.
- **VMM/IOMMU Enforcement**: The IOMMU (Intel VT-d, AMD-Vi, ARM SMMU) is strictly configured by the microkernel to prevent malicious or malfunctioning devices from performing DMA (Direct Memory Access) attacks on arbitrary physical memory.
- **IPC Wrapping**: Applications do not talk to hardware. They talk to the Driver Task via fast Synchronous IPC or URPC ring buffers.
- **Recovery**: If a network driver crashes due to a vulnerability, the microkernel merely tears down the isolated task space and transparently restarts the driver. The core OS remains entirely unaffected.
