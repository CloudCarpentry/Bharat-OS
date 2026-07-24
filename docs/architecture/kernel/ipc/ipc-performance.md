---
title: IPC Performance
status: Proposed
owner: Documentation Working Group
last_updated: 2026-04-25
tags:
  - docs
  - architecture
  - kernel
see_also:
  - README.md
---
# IPC Performance

## Overview
Because Bharat-OS is a microkernel, almost all OS services (file systems, networking, drivers) run in isolated user-space domains. Thus, IPC performance is the most critical bottleneck.

This document outlines the performance characteristics and design goals of the Bharat-OS IPC mechanisms.

## Synchronous Endpoint IPC Latency (The Fast Path)
The primary metric for microkernel performance is the round-trip time (RTT) of a synchronous ping-pong message between two user-space tasks on the same CPU core.

### The Fast Path Design
To minimize latency, the `ipc_endpoint_send` and `ipc_endpoint_receive` system calls are highly optimized:
1.  **Register-Only Transfer:** The payload (typically 4 to 8 machine words) is passed entirely in the CPU registers (e.g., `r4`-`r11` on ARM, `r8`-`r15` on x86_64). There is no copying to or from memory buffers.
2.  **Direct Context Switch:** When Task A sends a message to Task B (which is blocked waiting on the endpoint), the kernel *immediately* switches to Task B's address space and registers, bypassing the general scheduler queue. Task B is effectively scheduled by the IPC message.
3.  **Minimal State Saving:** Only the essential registers (the payload and the instruction/stack pointers) are saved and restored. Floating-point/SIMD state is saved lazily only if used.

### Theoretical Latency Bounds
A highly optimized, assembly-level IPC fast path on modern hardware (e.g., a 3GHz x86_64 or ARM Cortex-A CPU) can achieve a one-way context switch in under **200-300 clock cycles** (less than 100 nanoseconds).

## Asynchronous URPC Latency (The Multikernel Path)
For cross-core communication, synchronous IPC is too slow due to cache coherence overhead and locking. Bharat-OS uses lockless URPC ring buffers in shared memory.

### The URPC Design
1.  **Shared Memory Ring:** Core 0 and Core 1 share a cache-aligned memory region containing a circular buffer of messages.
2.  **Lockless Atomics:** The sender (Core 0) updates the `tail` index using an atomic release operation (`__atomic_store_n(..., __ATOMIC_RELEASE)`). The receiver (Core 1) reads the `head` index using an atomic acquire operation (`__atomic_load_n(..., __ATOMIC_ACQUIRE)`). There are no spinlocks.
3.  **Polling vs. Interrupts:**
    -   *Polling (High Throughput/Low Latency):* Core 1 dedicatedly spins (polls) the ring buffer memory address. When a message arrives, the cache line is updated, and Core 1 processes it immediately (latency < 100 cycles, but consumes 100% CPU).
    -   *Interrupts (Power Efficient):* Core 1 sleeps (`WFI` / `MWAIT`). Core 0 writes the message and sends an Inter-Processor Interrupt (IPI) to wake Core 1. The IPI latency is significantly higher (often 1000-3000 cycles).

## Zero-Copy Goals (Large Payloads)
For transferring large amounts of data (e.g., network packets, video frames), register-based IPC or copying data into a URPC ring buffer is too slow.

Bharat-OS targets **Zero-Copy IPC** for bulk data:
1.  The sender allocates a shared memory page (or pool of pages).
2.  The sender writes the large payload into the page.
3.  The sender uses the fast-path synchronous IPC (or URPC) to send *only the Capability* (the pointer/index to the page) to the receiver.
4.  The receiver uses the Capability to map the page directly into its own address space. The data itself never moves.

This achieves memory bandwidth speeds (GB/s) regardless of the IPC overhead.