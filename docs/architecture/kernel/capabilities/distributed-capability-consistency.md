---
title: "Distributed Capability Consistency"
status: "Draft"
version: "0.1"
last_updated: "2024-03-24"
tags: ["architecture", "capability", "distributed", "multikernel", "consistency"]
---

# Distributed Capability Consistency

### Contract Status
- **Spec**: ✅ Documented and versioned
- **Implemented**: 🚧 Pending core/kernel/service behavior merge
- **Validated**: ❌ Pending stress/fault-injection tests


## 1. The Distributed Multikernel Context

Bharat-OS capabilities are conceptually sound within a single core's CSpace (rights attenuation, derivation trees, revocation). However, in a true multikernel architecture, capabilities are frequently delegated across core boundaries.

A capability is not fully secure if it is locally rigorous but globally provisional. When a capability crosses a core boundary, the system must guarantee:

- **Stale Epoch Rejection:** The remote capability cannot be used after the original object is revoked.
- **Transactional Delegation:** A grant either fully succeeds on the remote core, or is safely rolled back locally.
- **Bounded Inconsistency:** If a core crashes or a capability is revoked during a cross-core transaction, the failure states must be deterministic and closed.

## 2. Canonical Capability Instance Identity

A capability transferred across a core boundary is an "instance" of the original local capability. It must be universally identifiable to prevent replay attacks and allow global revocation.

Every capability instance sent over uRPC MUST include:

```c
typedef struct cap_instance_id {
    uint32_t origin_core;       // The core that authoritative owns the object
    uint64_t object_id;         // The unique ID of the underlying kernel object
    uint32_t slot_gen;          // Generation number of the local CNode slot
    uint32_t rights_digest;     // Hash or bitmask of the granted rights
} cap_instance_id_t;
```

This ensures that:
- Stale capability slots reused for new objects will have a new `slot_gen`.
- Capabilities granted with reduced rights have a distinct `rights_digest`.
- A remote core attempting to forge an object ID cannot impersonate the `origin_core`.

## 3. Delegation Protocol

Cross-core capability delegation must be a two-phase commit over the uRPC protocol engine.

### Transaction States

The lifecycle of a cross-core capability grant transitions through the following states:

1.  `PROPOSED`: The origin core allocates a local tracking node and sends the capability over uRPC.
2.  `INSTALLED_REMOTE`: The destination core allocates a CNode slot, installs the capability, and queues an ACK.
3.  `ACKED`: The origin core receives the ACK and marks the delegation as active. The remote core can now invoke the capability.
4.  `REVOKING`: The origin core initiates a revoke, incrementing the epoch and broadcasting invalidation messages.
5.  `DEAD`: The remote core acknowledges the invalidation, and the origin core frees the tracking node.

## 4. Revocation Epochs

To protect against cross-core race conditions (e.g., a "Revoke" message is sent, but the destination core concurrently sends an "Invoke" message based on the now-stale capability), Bharat-OS uses **Revocation Epochs**.

-   **Global/Object Monotonic Counter:** Every kernel object or capability tree maintains a `revocation_epoch` counter.
-   **Epoch Attached to Messages:** Every uRPC message invoking a capability MUST carry the sender's known `capability_epoch`.
-   **Epoch Validation:** When the origin core processes an invocation, it compares the message's `capability_epoch` with the object's current epoch.
    -   If `msg.capability_epoch < object.epoch`: The invocation is rejected as stale (`ERR_CAP_REVOKED`).
-   **Epoch Increment:** Every `Revoke` operation on an object increments its epoch *before* sending out invalidation messages to remote cores.

## 5. Failure Handling Semantics

In a distributed environment, the uRPC layer may experience timeouts or core crashes. The capability system must gracefully handle these events.

| Failure Scenario | Resolution Behavior |
| :--- | :--- |
| **Grant sent, no ACK received (Timeout)** | The origin core must rollback the local tracking node. The remote core, if it eventually receives the delayed message, must reject it based on the uRPC transaction state. |
| **Revoke sent, no ACK received (Timeout)** | The origin core forces an invalidation. The epoch is already incremented, so any delayed invocations from the unresponsive core will be rejected. |
| **Remote Core Crash (Receiver)** | The origin core, upon detecting the crash (e.g., via watchdog), invalidates all capabilities delegated to that core. |
| **Origin Core Crash (Sender)** | The crashing core's objects are inherently destroyed. The remote cores must invalidate all foreign capabilities originating from that core unless a designated supervisor explicitly re-homes them. |
