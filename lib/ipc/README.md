# lib/ipc

## Purpose
Higher-level IPC wrapper above raw syscall/URPC. Reduces boilerplate across every service by defining endpoint request/reply, async URPC channels, message schemas, and timeout/cancellation behaviors.

## Dependencies
- **May depend on:** `lib/cap` (if absolutely needed for passing capabilities), standard C library headers.
- **Must not depend on:** `lib/runtime`, `services/*`, `subsys/*`, `ui/*`.
- **Must not use:** direct kernel-private headers.

## Planned Public API
- `bharat_ipc_endpoint_t`
- `bharat_ipc_msg_header_t`
- `bharat_ipc_send`
- `bharat_ipc_recv`
- `bharat_ipc_call` (Request-reply wrapper)

## Immediate TODOs
- Implement send/recv stubs using underlying URPC implementation.
- Introduce zero-copy buffer descriptors.
- Support timeout semantics.

## Status
Stub.
