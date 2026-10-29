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
- Expand async APIs and cancellation model.
- Introduce zero-copy buffer descriptors.
- Add richer capability-transfer envelope fields for public API.

## Status
V1 supports data-only send/receive over the existing endpoint syscall path.
Receive clears serialized `capability_transfer` and `reply_endpoint` integers:
these are untrusted wire data and do not establish authority. The wrapper declines
incoming capabilities through the kernel's NULL cap sink and rejects the transfer
flag. Unknown flags are rejected; decoded outputs publish only after validation.

`bharat_ipc_call[_ex]` returns `BHARAT_IPC_STATUS_ERR_UNSUPPORTED` before allocating
an endpoint, sending work, or changing outputs. The current ABI lacks authenticated
reply-role provenance and capability/endpoint release, so a fresh endpoint per RPC
would leak on success, send failure, or timeout. A versioned kernel receive envelope,
verified reply SEND authority, explicit cleanup, and safe endpoint destruction are
required before enabling RPC. Plain send/receive timeouts remain scheduler ticks;
`TIME_GET` returns nanoseconds and cannot supply their conversion implicitly.
Transport status decoding uses public native `BH_ERR_*` values, after the
kernel's IPC-to-native translation, rather than the distinct internal IPC enum.
Service status aliases currently coalesce timeout/interruption into internal
error; no more precise public service status is invented here.

The wire unit tests verify refusal, serialized-handle stripping, NULL cap sink,
unchanged outputs on failure, and 10,000 resource-free RPC rejections. They mock
transport and do not qualify authenticated IPC or guest service integration.
