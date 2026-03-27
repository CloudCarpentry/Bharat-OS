---
title: Bharat-OS Native UAPI Roadmap
status: active
owner: Architecture Team
reviewers: ["Core Team"]
version: 1.0
last_updated: "2024-03-27"
tags: ["architecture", "uapi", "native", "capabilities"]
---

# Bharat-OS Native UAPI (Application Binary Interface) Roadmap

## 1. Introduction

Bharat-OS is built upon a capability-oriented microkernel architecture. To fully leverage the security, isolation, and performance characteristics of this architecture, the system defines a **Native Bharat Personality** and corresponding Application Binary Interface (ABI).

The guiding principle for application interfaces in Bharat-OS is:

> **Kernel mechanisms are native. Services are native. UAPI is native. Compat personalities translate into native.**

This document outlines the roadmap for the **Native UAPI**, which resides in the `uapi/` tree. This is the explicit boundary for syscall headers, capability contracts, shared IPC structures, and stable ABI types.

Foreign ABIs (like Linux/POSIX) are **not** the core identity of the OS; they exist as compatibility layers (`personalities/compat/linux/`) that translate foreign semantics into this native UAPI.

## 2. The Native Contract Boundaries

The native contract is cleanly separated across the system architecture:

* **`kernel/`**: Provides mechanisms only (scheduling, memory, syscall/traps, capabilities, IPC/uRPC, faults, minimal coordination).
* **`uapi/`**: Defines the Bharat-native ABI and stable public contracts (process/thread syscalls, capability/object rights, IPC message formats, FS/service request/response contracts, native handle and error types).
* **`services/system/filesystem/`**: Owns namespace, paths, mount policy, FD/session policy, exposed via native IPC/uRPC.
* **`stacks/storage/`**: Owns composed storage internals.
* **`personalities/compat/linux/`**: Maps Linux `openat`, `read`, `write`, `stat`, etc. into the native FS/process/thread/service contracts defined in `uapi/`.

## 3. Native UAPI Domains

The Native UAPI encompasses several core domains, replacing global, ambient-authority APIs (like standard POSIX) with explicit, capability-scoped operations.

### 3.1 Handles and Capabilities
Native applications do not operate on raw file descriptors as global indices. Instead, they operate on capability handles.

* **Capability Operations**: Acquiring, copying, moving, and dropping capability handles.
* **Rights Narrowing**: Creating derived capability handles with a restricted subset of rights (e.g., deriving a read-only handle from a read-write handle).
* **Object Mapping**: Mapping handles to specific kernel or service objects (e.g., an IPC endpoint, a memory object, or a VFS node).

### 3.2 Process and Thread Primitives
A real Bharat-native application model exposes explicit lifecycle and execution control primitives.

* **Domain/Process Management**: Creating, destroying, and managing execution domains (processes).
* **Thread Management**: Native thread creation, execution, suspension, and join-like primitives (`kthread_t` management via UAPI).
* **Contexts and Faults**: Managing thread execution contexts, CPU affinity, and receiving asynchronous fault or exception messages.

### 3.3 Memory and Object Models
Native memory APIs focus on explicit mapping and sharing of memory objects, heavily utilized for zero-copy I/O.

* **Memory Objects (VMOs)**: Creating and managing Virtual Memory Objects.
* **Mapping**: Mapping VMOs into the address space with specific protection flags.
* **Shared Memory**: Using VMOs in conjunction with IPC for zero-copy data transfer.

### 3.4 IPC and Endpoints
All complex interactions, including filesystem and network operations, are mediated through the native IPC/uRPC model.

* **Endpoints**: Creating and binding to IPC endpoints for message passing.
* **Message Discipline**: Defining and enforcing stable, native message contracts and layouts for requests and responses.
* **Synchronous vs. Asynchronous**: Supporting both fast, synchronous IPC (for signaling/control) and asynchronous uRPC rings (for high-throughput data like storage or networking).

### 3.5 Native Filesystem and Object Access
The native FS contract is capability-oriented and service-routed, diverging significantly from ambient POSIX paths.

* **Object/Capability Based**: File and directory access requires possessing a capability handle granting rights to that object.
* **Service-Routed**: File operations (read, write, stat) are serialized as IPC/uRPC messages sent to the VFS service (`services/system/filesystem/`) owning the capability.
* **Namespace Mediation**: There is no global `/` by default. Applications only see the namespace explicitly granted to their sandbox via capabilities. Path resolution is relative to these granted namespace capabilities.

### 3.6 Namespace and Sandbox Semantics
Sandboxing is a first-class citizen, enforced by capability possession rather than user IDs or access control lists (ACLs).

* **Domain Creation**: Creating new sandboxed execution domains with a strictly defined set of initial capabilities.
* **Namespace Delegation**: Granting a subset of a parent domain's namespace to a child domain.
* **Resource Limits**: Associating resource limits and usage quotas with specific sandboxes or domains.

## 4. Relationship to `compat/linux/`

The Linux compatibility layer (`personalities/compat/linux/`) is built strictly *on top* of the Native UAPI.

For example, when a Linux binary invokes the `openat` syscall:
1. The trap is caught and routed to the Linux personality layer.
2. The Linux personality translates the POSIX path and flags into a native IPC request.
3. The IPC request is sent via the Native UAPI to `services/system/filesystem/`, carrying the process's namespace capability.
4. The VFS service resolves the path, allocates a native object handle, and returns it.
5. The Linux personality maps this native capability handle to an integer file descriptor (FD) and returns it to the Linux application.

This ensures the native ABI remains pure, while POSIX compliance is treated as a translation problem.

## 5. Next Steps / Phase Integration

1. **Storage/Filesystem Boundary Cleanup**: Complete the migration of VFS logic into `services/system/filesystem/` (Phase 2).
2. **Define UAPI Headers**: Solidify the `uapi/` headers for the primitives listed above (Capabilities, Threads, IPC, Memory).
3. **Wire Services to UAPI**: Connect `services/system/filesystem/` to accept requests structured via these new native UAPI contracts.
4. **Build Compat Adapters**: Implement `personalities/compat/linux/` as a strict consumer of the Native UAPI, providing standard POSIX ABIs for legacy applications.