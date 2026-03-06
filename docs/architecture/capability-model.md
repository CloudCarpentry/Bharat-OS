# Capability Model

## Overview

Bharat-OS enforces security through a mathematically verifiable Capability System. There are no global Access Control Lists (ACLs), user IDs, or root privileges inside the kernel.

## The Golden Rule: "Never Trust, Always Verify"

A capability is an unforgeable, kernel-managed token that pairs an object reference with a set of permitted operations (Read, Write, Execute, Grant). If a thread does not hold a capability to an object in its Capability Space (CSpace), the object does not functionally exist to that thread.

## Capability Operations

- **Invoke**: Perform an action on the object the capability points to (e.g., mapping a memory frame into an address space).
- **Grant**: Transfer a capability over an IPC Endpoint to another task.
- **Revoke**: Recursively invalidate a capability and all derivations of it.
- **Retype**: Convert an `Untyped` memory block capability into specific kernel objects (Threads, Endpoints, Frames).

## Security Benefits

- **Zero-Trust**: No ambient authority.
- **Verifiability**: The flow of authority can be graph-analyzed at compile time to mathematically prove isolation between critical domains (e.g., separating networking stacks from AI cryptographic enclaves).
