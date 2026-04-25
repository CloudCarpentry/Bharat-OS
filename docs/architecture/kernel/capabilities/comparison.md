---
title: Comparison: POSIX vs. seL4 vs. Bharat-OS
status: Draft
owner: Documentation Working Group
last_updated: 2026-04-25
tags:
  - docs
  - architecture
  - kernel
see_also:
  - README.md
---
# Comparison: POSIX vs. seL4 vs. Bharat-OS

## Overview
This document compares the access control and security models of traditional monolithic POSIX systems (like Linux), the formally verified seL4 microkernel, and the Bharat-OS multikernel approach.

## 1. POSIX (Linux, macOS, BSD)
**Model:** Discretionary Access Control (DAC) / Mandatory Access Control (MAC) with Ambient Authority.

*   **Authority:** Tied to the User ID (UID) and Group ID (GID) of the process.
*   **Object References:** File Descriptors (integers) mapping to kernel-space structs (`struct file`).
*   **Access Check:** Occurs at the time of opening an object (`open()`, `socket()`) against global policies (ACLs, SELinux). Subsequent uses (`read()`, `write()`) are fast.
*   **Resource Allocation:** The kernel implicitly allocates memory for page tables, file structs, and network buffers when user-space requests them.
*   **Weakness:** Confused Deputy Problem. If a highly privileged process (root) is tricked into opening a file for a malicious user, the access check passes because the *process* has ambient authority.

## 2. seL4
**Model:** Pure Capability-Based Security with strict Resource Delegation.

*   **Authority:** Tied exclusively to the capabilities held in the Thread's CSpace. No UIDs or ambient authority exist.
*   **Object References:** Capability Pointers (CPTRs) indexing into a CNode graph.
*   **Access Check:** Hardware/software checks the validity and rights of the capability on *every* invocation.
*   **Resource Allocation:** The kernel *never* implicitly allocates memory. User-space must explicitly provide Untyped memory capabilities to the kernel to create *any* object (TCBs, Endpoints, Page Tables).
*   **Strength:** Mathematically proven isolation. Immunity to the Confused Deputy Problem.

## 3. Bharat-OS
**Model:** Multikernel Capability-Based Security (Heavily inspired by Barrelfish and seL4).

*   **Authority:** Tied exclusively to capabilities in the Task's CSpace.
*   **Object References:** Capability indices.
*   **Access Check:** Validated on invocation (e.g., `ipc_endpoint_send`, `vmm_map_page`).
*   **Resource Allocation:** Like seL4, relies on explicit Untyped memory delegation for kernel objects to guarantee resource bounds and prevent denial-of-service within the kernel.
*   **Multikernel Difference:** Because Bharat-OS treats cores as independent network nodes, capabilities must be safely transferred across cores. When a capability is sent over URPC to another core, a **Capability Wire Descriptor** (Capwire) is generated. The receiving core authenticates the descriptor and instantiates a local proxy capability.

### Summary Table

| Feature | POSIX (Linux) | seL4 | Bharat-OS |
| :--- | :--- | :--- | :--- |
| **Security Model** | DAC / MAC | Capability-Based | Capability-Based |
| **Authority** | Ambient (UID/GID) | Explicit (CSpace) | Explicit (CSpace) |
| **Object References** | File Descriptors | Capability Pointers | Capability Indices |
| **Kernel Memory Allocation** | Implicit (Kernel `kmalloc`) | Explicit (Untyped Retyping) | Explicit (Untyped Retyping) |
| **Cross-Core Coordination** | Shared Memory + Spinlocks | Shared Memory + Spinlocks | Lockless URPC Message Passing |
| **Cross-Core Capabilities** | N/A (Shared Kernel State) | N/A (Shared Kernel State) | Wire Descriptors (Capwire Proxy) |