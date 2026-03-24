---
title: "Service Incarnation Model"
status: "Draft"
version: "0.1"
last_updated: "2024-03-24"
tags: ["architecture", "ipc", "urpc", "multikernel", "identity", "restarts"]
---

# Service Incarnation Model

## 1. The Restart-Safety Problem

In a multikernel microservice architecture, services (such as a device driver, a network manager, or a file system) can crash or be intentionally restarted to recover from a fault domain error.

If a service crashes and restarts, it often re-registers with `namesvc` and may even reclaim the same logical endpoint ID or uRPC channel slot.

The danger arises from **stale handles**:
1. Client A resolves `netmgr` and gets an endpoint handle.
2. `netmgr` crashes and restarts, reclaiming the same endpoint ID.
3. Client A sends a message using its old handle.
4. The new `netmgr` instance receives a message intended for its predecessor, leading to security breaches (e.g., performing actions based on an outdated capability or session state) or memory corruption.

## 2. The Incarnation Solution

To solve this, Bharat-OS explicitly separates object/service identity from session/transport identity by introducing an **Incarnation Generation** (`endpoint_gen`).

Every cross-core message or endpoint invocation MUST bind to a specific incarnation of a service.

```
endpoint_identity = {
    service_id,
    incarnation_id
}
```

### Definitions

*   **`service_id` (Logical ID):** A stable, universally unique identifier for the service itself (e.g., the concept of the "Network Manager"). This ID survives restarts.
*   **`incarnation_id` (Generation):** A monotonically increasing counter (or cryptographically secure random nonce) representing a specific execution run of the service.
    *   It **MUST** increment every time the service restarts.
    *   It is ephemeral and tied to the service's current lifecycle.

## 3. Enforcement Rules

1.  **Transport Inclusion:** Every `urpc_msg_hdr` and endpoint IPC message MUST include the target `endpoint_gen`.
2.  **Stale Rejection:** The receiving core or service dispatcher MUST validate the incoming `endpoint_gen` against the service's current incarnation.
    *   If `msg.endpoint_gen != current.endpoint_gen`, the message is instantly rejected with `FAILED_STALE_ENDPOINT`. The payload is never processed.
3.  **Namesvc Binding:** When a client queries `namesvc` to resolve a service, the returned capability or endpoint descriptor MUST embed the current `incarnation_id`.
4.  **Client Re-resolution:** Upon receiving a `FAILED_STALE_ENDPOINT` error, a robust client should assume the service has restarted. It must query `namesvc` again to acquire the new handle and `incarnation_id` before attempting to re-establish a session.

## 4. Relationship to Capabilities

While capabilities use Revocation Epochs (`capability_epoch`) to ensure an object's authority hasn't been revoked, Incarnation IDs (`endpoint_gen`) ensure the *transport destination* is still the expected entity. Both are required for a secure distributed OS.