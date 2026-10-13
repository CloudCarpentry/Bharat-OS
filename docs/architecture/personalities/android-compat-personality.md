---
title: Android Compatibility Personality
status: active
owner: Architecture Team
last_updated: 2026-04-25
tags:
  - docs
  - architecture
  - personalities
see_also:
  - README.md
version: 1.0
---
# Android Compatibility Personality

The Android Compatibility Personality is designed to provide application ecosystem leverage for mobile, embedded, and consumer profile use cases. It aims to host Android application frameworks and runtimes above the Bharat-OS native core.

## 1. Scope and Purpose

*   **Goal:** Enable running Android apps (APK/AAB) and reusing Android ecosystem components (like SurfaceFlinger equivalents or media stacks) with minimal modification.
*   **Target Workloads:** GUI-heavy consumer apps, mobile middleware, and embedded Android use cases.

## 2. ART, Syscall ABI, and Runtime Expectations

*   **Syscall Trap ABI:** Android Bionic libc invokes the standard Linux system call ABI for the corresponding CPU architecture (`svc #0` on ARM64/ARM32, `ecall` on RISC-V 64/32, `syscall` on x86_64). Full register mappings are documented in [`linux-android-syscall-arch.md`](./linux-android-syscall-arch.md).
*   **Runtime Hosting:** The personality hosts the Android Runtime (ART) by providing required native memory mappings, threading behaviors (Bionic TLS, robust futexes, stack limits), and JNI environments required by ART.
*   This relies heavily on the shared **Runtime Hosting Layer** (`interface/uapi/runtime/`) and Linux personality compatibility core for standard process and memory hooks.

## 3. Binder IPC Strategy

*   **No In-Kernel Binder:** Bharat-OS will not implement Binder in the kernel.
*   **User-Space Translation:** The personality must implement a user-space Binder emulation layer or an adapter that translates Binder transactions into Bharat-OS native URPC messages.
*   Android services (e.g., ActivityManager) will run as isolated user-space processes communicating over this emulated Binder/URPC bridge.

## 4. HAL Adaptation (Display, Input, Audio, Sensors)

*   The Android HAL (Hardware Abstraction Layer) will not communicate directly with Bharat-OS kernel drivers.
*   The personality provides an **Adapter HAL**:
    *   **Graphics (Gralloc/Surface):** Maps to the Bharat-OS native display server and shared memory arenas.
    *   **Audio/Input:** Translates Android AudioFlinger and InputManager requests into native Bharat-OS service IPC.
    *   **Sensors:** Bridges Android sensor events from the native sensor service.

## 5. Packaging and Deployment Flow

*   Android APKs remain the deployment unit.
*   The `bharat-pkg` tool (or a dedicated Android personality installer) parses the `AndroidManifest.xml` and translates its permissions into Bharat-OS capabilities.
*   The application runs inside a native Bharat-OS sandbox, meaning Android permissions are strictly enforced by the native capability subsystem.

## 6. Compatibility Limits

*   We do not aim to support full custom Android ROM features or deep system-level Android modifications.
*   Root apps or apps expecting to drop into a raw Linux shell will not work.
