---
title: Access Control Model
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
# Access Control Model

## Overview
Traditional monolithic operating systems like Linux rely on Discretionary Access Control (DAC) through file permissions (e.g., `rwxr-xr-x`) or Mandatory Access Control (MAC) systems (e.g., SELinux, AppArmor). These rely on an "ambient authority" context: the process itself runs as a user ID (UID).

Bharat-OS uses a pure Capability-Based Access Control model. This eliminates the concept of UIDs, GIDs, and ambient authority entirely within the kernel.

## No Ambient Authority
-   **Definition:** "Ambient Authority" means a task automatically has permission to do something because of *who it is* (e.g., "I am root," or "I am in the wheel group").
-   **Problem:** If a program running as "root" is compromised (e.g., via a buffer overflow), the attacker inherits the ambient authority and can access *any* file or device.
-   **Solution:** In Bharat-OS, a task only has permission to do something if it explicitly possesses a capability (a cryptographic token) for that specific object.

## Capability as the Security Token
A capability intrinsically combines **who** is allowed access with **what** they are allowed to access and **how** they can access it.

1.  **Who:** The task that holds the capability in its CSpace.
2.  **What:** The object reference embedded in the capability (e.g., physical memory address 0x1000).
3.  **How:** The rights mask embedded in the capability (e.g., `READ | WRITE`).

## Access Validation
When a task invokes a system call, the kernel validates the access in a single step:

1.  **Lookup:** The task provides an index (CPTR) into its CSpace. The kernel looks up the slot.
2.  **Type Check:** The kernel verifies the capability type matches the requested operation (e.g., `CAP_TYPE_FRAME` for a mapping operation).
3.  **Rights Check:** The kernel verifies the capability rights mask contains the necessary permissions (e.g., `CAP_RIGHT_WRITE`).
4.  **Execute:** If valid, the kernel performs the operation on the referenced object.

## Contrast with POSIX Models
-   **POSIX:** `open("/etc/passwd", O_RDWR)` -> Kernel checks UID against file permissions in the VFS. If valid, it returns an integer file descriptor.
-   **Bharat-OS:** Task has a capability for the `/etc/passwd` file server endpoint. It sends an IPC message: `Give me read access to /etc/passwd`. The server replies with a new Endpoint capability representing the open file stream. The kernel only checked if the task had a valid capability to *talk* to the server.

The access control policy is pushed completely into user space (e.g., the File Server enforces DAC or MAC), while the kernel only enforces the capability mechanisms.