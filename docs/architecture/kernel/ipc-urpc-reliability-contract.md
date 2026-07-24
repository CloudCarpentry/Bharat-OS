# IPC/URPC Reliability Contract

Status: Partial / IPC0 baseline

## Goals
- Explicit validation contract for IPC messages.
- Deterministic error/status results for IPC operations.
- Safe URPC channel state transitions with validation.
- Minimal backpressure admission control based on payload and traffic type.

## Non-goals
- No syscall changes.
- No scheduler changes.
- No full cross-node routing.
- No full distributed message broker.
- No capability enforcement rollout.

## IPC Contract Validation
The IPC contract validator (`bharat_ipc_contract_validate_ex`) enforces:
- Proper header and interface versioning.
- Opcode range validation.
- Payload size and optional alignment constraints.
- Mandatory and allowed flag bitmasks.
- Rejection of reserved or unsupported flags.

## IPC Status Mapping
A canonical mapping from `bharat_status_t` to human-readable strings is provided via `bharat_ipc_status_to_string()`. This ensures consistent logging and debugging across the kernel and userspace services.

## URPC Channel Reliability
The URPC channel layer now enforces a strict state machine for channel lifecycles:
- **CLOSED -> BINDING/BOUND**: Initiation of a channel.
- **BINDING -> BOUND/ERROR/CLOSED**: Result of a binding request.
- **BOUND -> CLOSED/ERROR**: Termination or failure of a bound channel.
- **ERROR -> CLOSED**: Mandatory reset before retry.

Transitions are validated via `urpc_channel_transition_allowed()`. Self-bind and duplicate bind attempts are explicitly rejected.

## IPC Route Admission
The `ipc_route_admit()` function provides an early-rejection mechanism for IPC traffic. It evaluates whether a message can be admitted based on:
- Traffic type (Control, Bulk, etc.)
- Payload size relative to transport limits (Endpoint vs. URPC).
- Core routing (Local vs. Cross-core).

This serves as the foundation for full backpressure support in future IPC iterations.
