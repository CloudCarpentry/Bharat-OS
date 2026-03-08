# Android Personality: Core Kernel Changes Required

## Introduction

The Bharat-OS core kernel is designed to be multi-personality (Linux, Windows, macOS, Android) and distributed. Supporting an Android personality requires specific architectural provisions in the core kernel to prevent the core from becoming bloated with Android-specific concepts (such as Binder or ashmem).

This document details the required changes to the core kernel to efficiently run the Android personality subsystem.

## 1. Personality-Neutral Process Model

To handle Android semantics natively, the core kernel process model (`kernel/include/sched.h`) has been updated to include a `personality` field in `kprocess_t` and `kthread_t`.

- **Syscall Dispatch:** The trap/syscall layer must route incoming calls based on the `personality` tag, redirecting Android-specific system calls or POSIX deviations to the Android compatibility subsystem.
- **Exception/Signal Handling:** The kernel must provide personality-specific exception/signal translation hooks to map native trap states to Android-specific (or Linux-like) signal structures.

## 2. Object and Handle Model

Android relies heavily on an object-and-handle driven runtime (e.g., Binder nodes, service manager handles, ashmem fds).

- **Generic Kernel Objects:** The core kernel must expose generic object types for processes, threads, address spaces, shared memory objects, message endpoints/channels, and event/wait objects.
- **Translation:** The Android personality subsystem maps these generic objects to Android-specific handles. A capability-based transfer in the core IPC mechanism implicitly transfers the authority of the object, mimicking Binder handle delegation.

## 3. IPC Changes for Binder-Style Behavior

Binder is a synchronous, object-oriented IPC system. Hardwiring Binder directly into the core kernel violates the Bharat-OS distributed architecture.

- **Required Core IPC Primitives:**
  - Synchronous request/reply IPC (for RPC-like Binder transactions).
  - Asynchronous one-way messaging.
  - Transfer of handles/capabilities/object references.
  - Zero-copy or low-copy payload paths.
  - Shared-memory assisted transfers for larger payloads (transaction buffers).
- **Distributed Topology:** The generic IPC must support cross-core and cross-node routing. Binder-like transactions from the Android personality will use these primitives, ensuring high throughput across multikernel topologies.

## 4. Shared Memory (ashmem path)

Android `ashmem` must not be a one-off allocator hacking the memory manager.

- **Core Support Needed:**
  - Named or anonymous shared memory objects.
  - Fd-attachable memory regions (or handle-backed regions).
  - Seal/protection flags.
  - Efficient mapping into multiple address spaces.
- **Abstraction:** The Android subsystem (`ashmem_compat.c`) simply provides an ashmem-compatible interface (ioctl/mmap) over the core's native memory objects. Future enhancements for DMA-safe or device-shareable memory classes will integrate seamlessly here.

## 5. Service Discovery and HAL Support

The core kernel should not know Android HAL details.

- **Core Primitives:** The kernel must provide generic service registration, capability-checked discovery, endpoint publication, and device/service namespaces.
- **Integration:** The Android Service Manager translates Android HAL-facing service lookups into Bharat-OS capability endpoint queries.

## 6. Distributed Scheduler and IPC Interaction

Since Bharat-OS is distributed and efficiently multithreaded, Android service chatter (which can be a significant bottleneck if poorly managed) must be optimized.

- **Locality-Aware Routing:** Binder-like calls should prefer locality-aware routing, minimizing cross-core overhead.
- **Locking:** Cross-core IPC must avoid global locks. The kernel must not assume one giant monolithic Binder lock.
- **Thread Wakeups:** IPC wakeups must be efficient and ownership-aware. Priority inheritance or priority-aware IPC (critical for Android real-time responsiveness) should be natively supported by the core scheduler to handle inverted priority conditions during Binder transactions.
- **Sharding:** Use per-core queues or sharded object registries to scale Binder object registries across the machine.
