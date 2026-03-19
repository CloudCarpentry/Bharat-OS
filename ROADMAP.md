# Bharat-OS Evolution Roadmap

This document details the step-by-step evolution of Bharat-OS from a basic microkernel bootloader up to a fully distributed, multikernel architecture capable of running advanced AI and GUI workloads.

## Phase 1: Core Multikernel Foundation (Current)
- **Per-CPU State Management:** Isolate runqueues, capability tables, and memory shards to individual cores.
- **Multicore Bootstrap & Monitor Processes:** Spawn privileged Monitor domains per core to handle cross-core requests.
- **Dynamic Lockless URPC Channels:** Establish lockless messaging rings between cores.
- **Cross-Core Capability Operations:** Enable capability delegation and revocation across the multikernel using asynchronous URPC messages.
- **Message-Based TLB Shootdown:** Replace inter-processor interrupts (IPI) with non-blocking URPC shootdowns for virtual memory consistency.
- **Basic System Knowledge Base (SKB):** Boot-time hardware topology discovery for optimized transport routing.

## Phase 2: Device Specialization & Edge UI
- **Subsystem Isolation:** Move non-essential drivers (network, storage) to isolated capability-mediated user-space domains.
- **Secure Boot & OTA Validation:** Enable cryptographically signed bootchains with hardware ROT (Root of Trust).
- **Framebuffer & Input Subsystem:** Implement 2D renderer and input multiplexing for robust embedded UIs without a heavy compositor.
- **Deterministic AI Scheduling Heuristics:** Collect hardware PMCs natively and train local predictive runqueue balancing for edge computing.

## Phase 3: Cloud, Accelerators, & Datacenter
- **NUMA-Aware Demand Paging:** Improve memory allocation and locality metrics via Bharat-Cloud memory policy extensions.
- **Heterogeneous Accelerator Subsystem:** Incorporate DMA, NPU, and GPU drivers. Establish zero-copy pipelines directly to accelerator domains.
- **High-Speed Networking:** Implement a full IP/TCP stack, establishing lockless kernel-bypass paths for VirtIO and standard NICs.
- **Scale-Out Multikernel Messaging:** Extend URPC primitives to transmit messages across network boundaries for true distributed execution.

## Phase 4: Advanced UX & Formally Verified Core
- **Hardware-Accelerated Compositor:** Migrate from standard framebuffer to capable desktop GUI compositor.
- **Isabelle/HOL Proof Foundations:** Integrate seL4-style verification tools to formally prove correctness of the URPC and capability isolation boundaries.
- **Ecosystem Personalities (Linux/Android):** Mature translation layers for POSIX compatibility, allowing unmodified applications to execute in secure enclaves.
