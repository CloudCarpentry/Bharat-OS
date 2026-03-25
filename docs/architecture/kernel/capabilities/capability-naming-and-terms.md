# Capability Naming and Terms

This document specifies the canonical naming conventions for capability types, rights, and related distributed-consistency primitives in Bharat-OS.

## Types and Rights

All capability types must be prefixed with `CAP_TYPE_`.
All capability rights must be prefixed with `CAP_RIGHT_`.

Avoid adding duplicate type-specific rights when generic ones can be applied. For example, prefer `CAP_RIGHT_READ_STATS` over `CAP_RIGHT_ACCEL_READ_STATS` if the concept is generally applicable.

## Distributed Identity

When delegating capabilities over uRPC, or managing capability lifecycle across cores, the following primitives must be used:

- `cap_instance_id_t`: Carries the canonical object and origin identity for a capability crossing a core boundary. It prevents forging the origin and helps identify distinct delegations of the same object.
- `revocation_epoch`: A monotonically increasing value tied to a capability or object. It is incremented on revocation to reject stale invocations immediately. On the wire in uRPC headers, this is often represented as `capability_epoch`.

## Endpoints and Services

When dealing with IPC/uRPC endpoints:

- `endpoint_gen`: The endpoint generation. It is incremented every time an endpoint is re-initialized (e.g., service restart), serving to reject stale capabilities that point to a previous incarnation.
- `service_id`: The canonical, stable identifier for a service.
