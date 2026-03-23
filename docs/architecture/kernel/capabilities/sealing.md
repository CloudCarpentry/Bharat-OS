# Sealing

## Overview
In a capability-based system, **Sealing** provides a mechanism for safely passing opaque objects or states between mutually untrusting parties.

## The Problem
Imagine Task A creates a complex object (e.g., a state machine for an AI governor, or a file handle pointing to a specific internal state) and wants to give Task B a reference to it. Task A cannot just give Task B a raw memory capability, as Task B could alter the internal state.

Task A also cannot just give Task B an Endpoint to call it, as Task A must keep track of Task B's state, leading to a state explosion on the server-side.

## The Solution: Sealed Capabilities
Sealing solves this by cryptographically or logically "locking" a capability so its contents (the object reference and rights) cannot be read or modified, but the token itself can still be passed around or invoked according to specific rules.

### How it Works
1.  **Creation:** Task A creates an object (e.g., an Untyped memory block) and populates it with data.
2.  **Sealing:** Task A uses a special capability (a Sealer Capability or "Minting" capability) to convert the object's capability into a Sealed Capability.
3.  **Transfer:** Task A gives this Sealed Capability to Task B (e.g., over IPC).
4.  **Usage:** Task B cannot read or write the data within the object. However, Task B can pass the Sealed Capability *back* to Task A (or another authorized party) via an IPC message.
5.  **Unsealing:** When Task A receives the Sealed Capability, it uses an Unsealer Capability (which only it possesses) to verify its authenticity and access the underlying object reference.

## Sandboxing and State Management
Sealing is heavily used for:
-   **Client State Cookies:** A server (e.g., VFS) can seal a client's session state into a capability and give it to the client. The client stores it but cannot read it. When the client makes a request, it passes the sealed capability back, and the server unseals it to retrieve the session state. This allows the server to be completely stateless.
-   **Delegation of Authority:** A parent task can seal a set of capabilities into a bundle and give it to a sandboxed child task. The child can only use the bundle as a whole by passing it to an authorized supervisor.

## Hardware Support (CHERI)
On architectures like CHERI (Capability Hardware Enhanced RISC Instructions), sealing is implemented directly in hardware registers. A CHERI capability is a 128-bit fat pointer that includes bounds, permissions, and a sealed bit. If the sealed bit is set, the pointer cannot be dereferenced or modified until it is unsealed with an appropriate authorization capability.