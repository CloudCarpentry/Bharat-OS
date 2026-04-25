---
title: uRPC Kernel Path
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
# uRPC Kernel Path

## Overview
While uRPC is inherently a "User-level Remote Procedure Call" designed to run largely without kernel intervention, the kernel remains involved in three critical paths: establishing the channel, resolving capabilities, and handling cross-core interrupts (IPIs) when polling is disabled.

## 1. Channel Establishment (Slow Path)
Before zero-copy uRPC can happen, the kernel must set up the shared memory ring buffer.
-   **User Action:** Client calls `urpc_channel_bind("bharat.vfs.v1")`.
-   **Kernel Action:** The kernel resolves the service name (usually via a local nameserver), allocates physical frames for the ring buffers, maps them into both the client's and the server's Address Spaces, and grants them the necessary capabilities. This path is slow but only happens once per connection.

## 2. Capability Resolution (Capwire Translation)
If a uRPC message contains capabilities (indicated by the `MSG_FLAG_HAS_CAPS` flag), the pure user-level bypass is broken. The kernel must get involved to maintain security.
-   **User Action:** Sender constructs a uRPC message and sets `cap_count = 1`. The sender places its local Capability Pointer (CPTR) into the message buffer.
-   **Kernel Trapping:** The user-level uRPC library executes a system call (e.g., `sys_urpc_send_caps`).
-   **Kernel Action:** The kernel reads the CPTR, verifies the sender has the right to delegate it, translates it into a Capwire descriptor (adding object IDs and nonces), overwrites the CPTR in the message buffer with the Capwire descriptor, and then optionally signals the remote core.
-   **Receiver Path:** On the receiving side, the kernel intercepts the incoming message, reads the Capwire descriptor, creates a local proxy capability in the receiver's CSpace, and overwrites the descriptor in the buffer with the new local CPTR before waking the receiver.

## 3. Interrupts and Wakeups
If the receiving core is in a low-power state (not polling the ring buffer), the sender must wake it up.
-   **Sender Action:** After writing the message to the ring buffer, the sender makes a fast system call: `sys_urpc_notify(channel_id)`.
-   **Kernel Action:** The kernel looks up the core assigned to that `channel_id` and sends a hardware Inter-Processor Interrupt (IPI) to that core.
-   **Receiver Kernel:** The receiving core handles the IPI, determines which uRPC channel triggered it, and wakes up any threads blocked waiting on that channel.

## Fast Path Summary (Pure Data Transfer)
If a message contains no capabilities and the receiver is actively polling:
1.  Sender writes to shared memory.
2.  Sender executes an atomic memory barrier (`__atomic_thread_fence`).
3.  Receiver reads from shared memory.
*Kernel involvement: Zero context switches, zero system calls. Pure memory bus latency.*