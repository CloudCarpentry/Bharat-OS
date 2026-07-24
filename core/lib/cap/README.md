# lib/cap

## Purpose
A first-class capability library. Provides typed capability handles, rights masks, delegation helpers, revocation calls, safe serialization for IPC, and debugging/introspection helpers. This forms the foundation for Bharat-OS's capability-oriented architecture.

## Dependencies
- **May depend on:** Nothing above it. Strictly freestanding or basic C standard types only.
- **Must not depend on:** `lib/ipc`, `lib/runtime`, `services/*`, `subsys/*`, `ui/*`.
- **Must not use:** direct kernel-private headers.

## Planned Public API
- `bharat_cap_handle_t`
- `bharat_cap_rights_t`
- `bharat_cap_is_valid`
- `bharat_cap_format`
- `bharat_cap_intersect_rights`
- (Future) Delegation API
- (Future) Serialization/Deserialization API

## Immediate TODOs
- Add real serialization/deserialization stubs for passing handles over IPC.
- Introduce concrete capability delegation wrappers mapping to syscall interfaces.

## Status
Stub.
