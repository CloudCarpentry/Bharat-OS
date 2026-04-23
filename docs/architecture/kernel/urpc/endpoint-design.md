# Endpoint Design

## Overview
In Bharat-OS, an Endpoint (`bh_endpoint_t`) is a capability-protected rendezvous point. For local (intra-core) communication, endpoints are implemented as synchronous wait queues in the kernel. For cross-core (multikernel) or network communication, **uRPC Channels** operate *underneath* or *alongside* these endpoints.

## The Problem: Capability Boundaries
How does Task A (on Core 0) send a capability to Task B (on Core 1)?
1.  **Local Case:** Task A invokes an Endpoint it shares with Task B. The kernel running on Core 0 removes the capability from Task A's CSpace and inserts it directly into Task B's CSpace (since both are managed by the same kernel instance).
2.  **Multikernel Case:** The kernel on Core 0 cannot directly modify Task B's CSpace, because that CSpace is managed by the kernel on Core 1. Core 0 and Core 1 do not share locks.

## The Solution: uRPC Proxies and Capwire

### 1. The Proxy Endpoint
To preserve the illusion of a local synchronous endpoint, Bharat-OS uses **Proxy Endpoints**.

-   Task A holds a capability to an Endpoint. As far as Task A is concerned, it's just a normal `bh_endpoint_t`.
-   However, the kernel on Core 0 knows that this Endpoint is actually a *proxy* pointing to a destination on Core 1.

### 2. The Capwire Translation
When Task A calls `ipc_endpoint_send()` on this proxy, the kernel on Core 0 intercepts the call.

1.  **Serialization:** The kernel converts the capability Task A is trying to send into a **Capability Wire Descriptor** (Capwire format). This descriptor contains the object's global ID, its rights, and its origin node.
2.  **uRPC Dispatch:** The kernel packages the message payload and the Capwire descriptor into a uRPC message (see `msg-wire-format-v1.md`).
3.  **Cross-Core Ring Buffer:** The kernel places this uRPC message into the shared memory ring buffer (`urpc_ring_t`) connecting Core 0 to Core 1.
4.  **Reception:** The kernel on Core 1 polls its ring buffer, sees the incoming uRPC message, extracts the Capwire descriptor, and creates a *new local proxy capability* in Task B's CSpace.
5.  **Delivery:** Finally, the kernel on Core 1 wakes up Task B, delivering the payload and the newly instantiated capability as if it had been sent locally.

### Endpoint Capabilities vs. uRPC Channels
-   **Endpoints (`CAP_TYPE_ENDPOINT`):** The *logical* destination for an IPC message. Capabilities control who is allowed to send or receive on this logical portal.
-   **uRPC Channels:** The *physical transport mechanism* (the lockless ring buffer in shared memory) connecting two points in the multikernel network. The kernel routes messages sent to Proxy Endpoints over these underlying channels.