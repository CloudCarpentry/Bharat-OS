Operating systems are strange beasts. They sit between **physics and abstraction**. On one side: electrons, buses, cache lines, interrupts. On the other: files, processes, sockets, GUIs. A good kernel architecture document is essentially a **map of that borderland**.

For a serious OS repository like **Bharat-OS**, three documents anchor the project:

• the **architecture map** (how the kernel is built)
• the **hardware support map** (what machines it runs on)
• the **research direction** (where the kernel should evolve)

Below are production-ready versions of those files.

---

# `KERNEL_ARCHITECTURE.md`

# Bharat-OS Kernel Architecture

## Overview

Bharat-OS is designed as a **modular, hardware-aware operating system kernel** capable of running on devices ranging from small embedded systems to large datacenter servers.

The architecture emphasizes:

- portability across CPU architectures
- modular subsystem design
- deterministic performance for real-time systems
- efficient hardware utilization

The kernel must operate efficiently on:

- embedded devices
- robotics systems
- drones
- automotive platforms
- networking appliances
- desktops and workstations
- datacenter infrastructure

---

# Kernel Architecture Layers

The Bharat-OS kernel is structured into several major layers.

```
Applications
   │
System Services
   │
Kernel Subsystems
   │
Hardware Abstraction Layer (HAL)
   │
Architecture-Specific Code
   │
Hardware
```

---

# Kernel Directory Layout

```
kernel/
    arch/
        x86_64/
        arm64/
        riscv64/
        shakti/

    hal/
        cpu/
        interrupt/
        timer/
        memory/

    mm/
        paging
        allocator
        vm

    scheduler/

    ipc/

    drivers/
        storage
        network
        gpu
        input
        sensors

    fs/
        vfs
        ext
        fat
        flashfs

    net/

    security/

    power/

    ai/

    debug/
```

---

# Hardware Abstraction Layer (HAL)

The HAL isolates architecture-specific hardware logic from the rest of the kernel.

Responsibilities include:

- interrupt controller configuration
- timer management
- CPU feature detection
- memory initialization
- low-level device interaction

Example:

```
hal/x86_64/apic.c
hal/arm64/gic.c
hal/riscv64/plic.c
```

---

# CPU Management

CPU subsystem handles:

- multi-core initialization
- CPU topology discovery
- SIMD/vector feature detection
- per-CPU data structures

Optimizations include:

- cache-line alignment
- NUMA awareness
- TLB tagging using ASIDs or PCIDs

---

# Memory Management

The memory subsystem is designed around a **3-plane Distributed VM Architecture** (see [ADR 008](../adr/008-distributed-vm-monitor-and-vm-spaces.md)) to natively support multikernel execution, diverse hardware profiles, and real-time constraints:

1. **Plane A — Canonical Distributed VM Model (`vm_space_t`)**: This plane is the source of truth. It manages address spaces, mappings, capabilities, sharing rules, and generations. It explicitly tracks timing classes (Best-Effort, Soft RT, Hard RT) and hardware profiles (MMU, MPU, MMU+DMA isolation).
2. **Plane B — URPC Monitor Protocol**: The distributed control plane. It propagates map/unmap/protect requests, handles remote invalidation (TLB shootdowns), and manages activation hints for thread migration across cores.
3. **Plane C — Local Hardware Realization (`arch_vm_ops_t`)**: The silicon-facing data plane. Each core locally executes page-table writes, CR3/TTBR/satp activation, local TLB invalidation, and page-fault decoding.

The memory subsystem handles:

- physical page allocation (PMM)
- canonical distributed virtual memory (`vm_space_t`)
- local page table realization
- kernel heap and slab allocation
- distributed memory coherence
- user memory and device DMA isolation

Features include:

- huge page support
- URPC-based remote TLB invalidation
- strict generation-based coherence for revocation
- NUMA memory allocation
- cache-aware allocation
- MPU/PMP fallback for hard-real-time and low-end systems

---

# Scheduler

The scheduler manages CPU time across tasks.

Scheduler implementations may include:

- Completely Fair Scheduler (desktop/server)
- Earliest Deadline First (real-time)
- priority scheduling
- energy-aware scheduling

The scheduler maintains:

- per-CPU run queues
- task priorities
- thread states
- load balancing across cores

---

# Interrupt Handling

Interrupt management includes:

- interrupt vector setup
- interrupt routing
- per-CPU interrupt dispatch
- deferred work queues

Supported controllers:

| Architecture | Controller |
| ------------ | ---------- |
| x86          | APIC       |
| ARM          | GIC        |
| RISC-V       | PLIC       |

Interrupt handlers must be minimal and defer heavy work to soft interrupt handlers.

---

# IPC (Inter-Process Communication)

IPC mechanisms include:

- message passing
- shared memory
- pipes
- event signals

Future designs may include:

- capability-based security
- microkernel-style messaging

---

# Filesystem Layer

Filesystem support is implemented through a **Virtual Filesystem Layer (VFS)**.

Supported filesystems may include:

- FAT
- EXT
- flash-optimized filesystems
- network filesystems

Embedded systems prioritize:

- minimal overhead
- flash endurance

Datacenter systems prioritize:

- reliability
- journaling
- scalability

---

# Networking Stack

The networking subsystem supports:

- TCP/IP
- UDP
- packet routing
- network drivers

High-performance features may include:

- zero-copy networking
- kernel bypass networking
- RDMA support

---

# Device Drivers

Drivers manage hardware interaction.

Categories include:

- storage devices
- network interfaces
- graphics hardware
- input devices
- sensors and robotics hardware

Drivers must be isolated to prevent kernel instability.

---

# Power Management

Power subsystem manages:

- CPU frequency scaling
- device sleep states
- thermal management
- battery management

Critical for:

- mobile devices
- embedded systems
- automotive systems

---

# AI Acceleration Layer

Bharat-OS may support hardware-accelerated AI workloads.

Possible integrations:

- NPU drivers
- GPU compute
- vector instruction acceleration

Potential kernel-level uses:

- anomaly detection
- predictive scheduling
- adaptive power management

---

# Security

Security subsystems include:

- process isolation
- memory protection
- secure boot
- cryptographic primitives

Future work may include:

- capability-based security models
- trusted execution support

---

# Debugging Infrastructure

Kernel debugging tools include:

- tracing systems
- kernel logging
- performance counters
- crash dumps

Tools used:

- QEMU
- GDB
- hardware debug probes

---

# `HARDWARE_SUPPORT.md`

# Bharat-OS Hardware Support

## Overview

Bharat-OS aims to support a wide variety of hardware platforms across multiple architectures.

The OS must operate on:

- embedded microcontrollers
- mobile processors
- workstation CPUs
- server-class processors

---

# Supported Architectures

| Architecture | Status       |
| ------------ | ------------ |
| x86_64       | primary      |
| ARM64        | primary      |
| RISC-V       | primary      |
| Shakti       | experimental |

---

# CPU Platforms

### x86 Platforms

Supported processors:

- Intel Core series
- AMD Ryzen
- Intel Xeon
- AMD EPYC

Key features used:

- APIC
- PCID
- AVX instructions

---

### ARM Platforms

Supported devices:

- Cortex-A series processors
- Apple Silicon
- Qualcomm Snapdragon
- ARM server processors

Key hardware features:

- GIC interrupt controller
- NEON vector units
- SVE extensions

---

### RISC-V Platforms

Supported platforms:

- SiFive boards
- Shakti processors
- QEMU RISC-V virtual machines

Key features:

- PLIC interrupt controller
- vector extension support

---

# Embedded Platforms

Bharat-OS targets embedded systems such as:

- robotics controllers
- industrial automation devices
- drones
- IoT devices

Supported components include:

- SPI
- I2C
- UART
- CAN bus
- sensor interfaces

---

# Automotive Systems

Automotive platforms may include:

- EV control systems
- vehicle infotainment systems
- driver assistance controllers

Important features:

- real-time scheduling
- safety-critical execution
- CAN and automotive Ethernet support

---

# Edge Computing Devices

Edge devices may include:

- IoT gateways
- industrial edge servers
- AI inference boxes

Important features:

- GPU/NPU support
- network acceleration
- containerized workloads

---

# Datacenter Platforms

Datacenter systems may include:

- multi-socket servers
- NUMA systems
- high-performance networking

Key requirements:

- scalability across many cores
- high memory bandwidth
- virtualization support

---

# Supported Hardware Interfaces

Bharat-OS must support common hardware interfaces:

- PCIe
- USB
- SATA
- NVMe
- Ethernet
- Wi-Fi

---

# `RESEARCH.md`

# Bharat-OS Research Directions

## Overview

Modern operating system research continues to evolve rapidly.

Bharat-OS should incorporate ideas from recent kernel and systems research.

---

# Multikernel Architecture

Traditional kernels treat the system as a shared memory machine.

Multikernel designs treat each CPU core as an independent system communicating via messages.

Example system:

Barrelfish OS

Advantages:

- improved scalability
- reduced contention
- better NUMA performance

---

# Hardware-Aware Scheduling

Future schedulers must understand:

- CPU topology
- cache hierarchy
- memory locality
- energy consumption

Example scheduling objective:

[
Cost = latency + cache_miss + energy
]

---

# Zero-Copy Networking

High-performance networking stacks minimize memory copying.

Examples include:

- DPDK
- netmap
- XDP

---

# Persistent Memory

Emerging hardware includes persistent memory technologies.

Examples:

- Intel Optane
- next-generation NVRAM

Filesystems must adapt to these technologies.

---

# AI-Assisted Kernel Optimization

AI techniques can improve system behavior.

Examples include:

- anomaly detection
- predictive resource allocation
- adaptive scheduling

Machine learning models can analyze system telemetry to optimize kernel behavior.

---

# Heterogeneous Computing

Modern systems combine multiple processor types.

Examples:

- CPUs
- GPUs
- NPUs
- DSPs

Operating systems must manage workloads across these heterogeneous processors.

---

# Energy-Aware Systems

Energy efficiency is critical for mobile and edge devices.

Kernel policies must consider:

- dynamic voltage scaling
- device sleep states
- workload consolidation

---

# Real-Time Operating Systems

Real-time workloads require deterministic scheduling.

Key approaches:

- earliest deadline first scheduling
- priority inheritance
- bounded interrupt latency

---

A repository with these three documents tells contributors:

“this is not just code — this is a **kernel architecture project**.”

If you want, the next useful document for Bharat-OS would be **`ROADMAP.md`**, which lays out the **step-by-step evolution of the kernel from bootloader → multitasking → networking → GUI → distributed systems**. That roadmap becomes the north star of the whole project.
