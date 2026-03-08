# Android Personality Architecture

## Introduction

The Bharat-OS multi-personality strategy does not bake monolithic compatibility subsystems into the core kernel. Instead, it maintains a small, verifiable, distributed kernel that exposes personality-neutral primitives (such as tasks, memory objects, and capabilities). Layered compatibility subsystems translate these core primitives into personality-specific abstractions (Linux POSIX, Windows NT, Android, macOS).

This document outlines the architecture for the **Android Personality Layer**, which relies on the existing Linux-compatible base primitives (process, memory, VFS, signals, futex, ELF) but extends them to satisfy Android's unique, heavily object-and-handle driven runtime (e.g., Binder IPC, ashmem, HALs).

## Core Philosophy: Reuse, Do Not Bypass

Android userspace historically depends on a Linux kernel substrate combined with Android-specific mechanisms. The Bharat-OS Android personality does not treat Android as a completely separate entity from Linux ABI foundations.

- **Core Kernel:** Remains personality-neutral.
- **Linux-Compatible Base:** Provides processes, memory mapping, fd/VFS, signals, futexes, ELF loading, timers.
- **Android Personality Layer:** Layers Binder semantics, ashmem/shared memory compatibility, property/service plumbing, and Android HAL-facing abstractions on top of the base primitives.

## Component Architecture

### 1. Personality Tagging

The Bharat-OS scheduler has been updated to support `personality` tagging at both the `kprocess_t` and `kthread_t` levels.

- **Subsystem Registration:** When an Android subsystem is created via `subsys_create(SUBSYS_TYPE_ANDROID, ...)`, any subsequent process spawned for this domain inherits the `SUBSYS_TYPE_ANDROID` personality.
- **Dispatch Hooks:** Exception/signal translation and syscall dispatch can inspect `kprocess_t->personality` to route Android-specific handling correctly, without polluting the generic fast paths.

### 2. Binder Compatibility Layer

Android heavily relies on Binder for object-oriented IPC. The architecture explicitly *avoids* hardwiring Binder semantics into the generic IPC subsystem.

- **Endpoint Translation:** The generic kernel IPC provides asynchronous messaging, capability/handle transfer, and zero-copy payloads. The Binder compatibility layer (`binder_compat.c`) exposes a `/dev/binder`-like character device and translates `ioctl` requests into these underlying Bharat-OS IPC capabilities.
- **Object Model:** A Binder node or handle maps directly to a Bharat-OS object capability. When an Android process transfers a Binder handle, the personality layer translates this into a capability delegation via Bharat-OS IPC.

### 3. Ashmem and Shared Memory

Android's `ashmem` (and modern memfd) provides named, anonymous shared memory regions that can be sealed and shared between processes securely.

- **No One-Off Allocators:** The core memory object model supports generic named/anonymous shared memory objects.
- **Ashmem Shim:** The Android personality layer (`ashmem_compat.c`) provides an ashmem-compatible abstraction on top of the core memory objects. This ensures that memory mapping, reference counting, and capability-based protection are handled safely by the core VMM, while Android processes see the familiar fd-backed or name-backed memory regions.

### 4. Service Manager and HAL Integration

The core kernel should not know Android HAL details. Instead, the Android personality translates service registration.

- **Capability-Checked Registry:** The Android Service Manager (`android_service_manager.c`) integrates with the core kernel's capability-checked endpoint discovery.
- **Lookup Translation:** When an Android client queries a service via a string name, the Service Manager looks up the corresponding capability object and grants a connection.

## Future Scaling

The Android personality is designed to be modular. Future work will expand these foundational seams to support:
- Full ART/Dalvik runtime hooks.
- A complete AIDL stack mapping to Bharat-OS endpoints.
- SELinux Android policy mapped to Bharat-OS capability matrices.
