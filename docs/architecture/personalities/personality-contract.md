---
title: Personality Contract
status: active
owner: Architecture Team
version: 1.0
---

# Personality Contract

### Contract Status
- **Spec**: ✅ Documented and versioned
- **Implemented**: 🚧 Pending kernel/service behavior merge
- **Validated**: ❌ Pending stress/fault-injection tests


This document is the root governance contract for all personality implementations within Bharat-OS. It defines the strict boundaries of what a personality is allowed to do, what it must delegate, and how it interacts with the underlying capability-oriented microkernel and services.

## 1. What a Personality Owns

A personality is an adapter layer responsible for translating an external Application Binary Interface (ABI) or runtime expectation into Bharat-OS native primitives. A personality owns:
*   **Application Lifecycle Translation:** Translating foreign process concepts (e.g., `fork`, Android Activity transitions) into Bharat-OS `spawn` and lifecycle messages.
*   **ABI/Syscall Emulation:** Implementing traps, syscall entries, or libc bridges that foreign binaries expect.
*   **Resource Mapping:** Mapping foreign identifiers (e.g., file descriptors, Android Binders) to Bharat-OS capability handles.
*   **Toolchain/Packaging Abstraction:** Defining how a foreign binary is packaged, deployed, and identified within its own ecosystem.

## 2. What a Personality Cannot Bypass

Personalities are strictly user-space components (or user-space service extensions). They **must not**:
*   **Bypass Capability Security:** A personality cannot access resources for which it (or its client process) does not hold a valid capability handle.
*   **Fork Kernel Architecture:** A personality cannot add architecture-specific drivers, kernel traps, or modify the core scheduler to achieve compatibility.
*   **Bypass the Driver Model:** Hardware access must always go through the unified device manager and service layer. Personalities cannot directly drive hardware (e.g., no raw memory mapping of device registers unless explicitly granted via a capability for an adapter service).
*   **Implement Monolithic Filesystems:** A personality must translate file operations to the Bharat-OS VFS IPC contracts. It cannot write its own raw block drivers.

## 3. Kernel and Service Interaction Rules

*   **Mechanism vs. Policy:** The kernel provides mechanisms (memory, threads, capabilities, IPC). Services provide policy (access control, routing, naming). Personalities are strictly clients to services.
*   **IPC Mapping:** Personalities must translate their native IPC (e.g., POSIX signals, D-Bus, Binder) into Bharat-OS URPC messages or local shared memory arenas coordinated via capabilities.
*   **Sandboxing:** Every personality runs within a standard Bharat-OS namespace and sandbox, constrained by the capability grant at launch.

## 4. Capability Enforcement Rules

*   **No Ambient Authority:** A personality cannot assume "root" or ambient file system access.
*   **Translation, Not Escalation:** If a Linux binary requests `open("/etc/passwd", O_RDONLY)`, the Linux personality must have been granted a namespace capability that includes the `/etc` equivalent, and must perform the lookup against the native VFS service.

## 5. ABI/API Stability Levels

*   **Native UAPI:** Strictly versioned and stable. Personalities rely on this.
*   **Personality Internal ABI:** Owned by the personality. The Linux personality may upgrade its `glibc` shim independently of the Native personality, provided it continues to map to the stable Native UAPI.
*   **Runtime Hosting:** Personalities interact with language runtimes via the Runtime Hosting layer, which provides a stable intermediate contract.

## 6. Runtime Hosting Dependency

All personalities must eventually target the shared **Runtime Hosting Layer** (`uapi/runtime/`, `lib/runtime/host/`) when integrating major managed runtimes (Java, Python, .NET, Node.js). Personalities should not implement bespoke bare-metal hooks for these runtimes if a shared abstraction exists.
