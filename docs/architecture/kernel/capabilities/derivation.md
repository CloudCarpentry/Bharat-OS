# Derivation and Revocation

## Overview
Capabilities are not static; they form a tree structure representing the flow of authority. A task holding a capability can **derive** a new one to grant to another task, and later **revoke** it.

## The Derivation Tree
When a task (Task A) delegates a capability (e.g., an IPC Endpoint) to another task (Task B), the kernel does not just copy the capability. It creates a new capability node in Task B's CSpace that points back to the original capability in Task A's CSpace.

This creates a **Derivation Tree**.

```mermaid
graph TD
    A[Task A: Root Endpoint Cap] -->|Delegates to B| B(Task B: Endpoint Cap)
    A -->|Delegates to C| C(Task C: Endpoint Cap)
    B -->|Delegates to D| D(Task D: Endpoint Cap)
    C -->|Delegates to E| E(Task E: Endpoint Cap)
```

## Minting and Attenuation
When deriving a capability, a task can **mint** a new version with *fewer* rights than it currently possesses. This is known as **Rights Attenuation** and is crucial for the Principle of Least Privilege.

- **Example:** Task A holds a Memory Frame capability with `READ | WRITE | EXECUTE` rights. It wants to give Task B access, but only to read the data. Task A mints a new capability with only the `READ` right and delegates it to Task B.
- **Rule:** The rights of a derived capability must be a subset of the parent capability's rights (`cap_transfer_rights_valid`). You cannot grant rights you do not have.

## Revocation
The derivation tree allows the kernel to perform recursive revocation.

- **Operation:** When Task A decides to **revoke** its original Endpoint capability, the kernel traverses the derivation tree.
- **Effect:** The kernel recursively deletes all derived capabilities. Task B, Task C, Task D, and Task E will immediately lose access to the Endpoint, their CSpace slots will be invalidated, and any future attempts to use their capabilities will result in an error (e.g., `-BH_EPERM` or `-BH_EINVAL`).

## Untyped Memory Derivation
Derivation also applies to Untyped memory. When a task splits a large block of Untyped memory (e.g., 2MB) into smaller blocks (e.g., 512x 4KB blocks), the smaller blocks are derivations of the larger block. Revoking the large block reclaims all the smaller blocks (and anything they were retyped into).