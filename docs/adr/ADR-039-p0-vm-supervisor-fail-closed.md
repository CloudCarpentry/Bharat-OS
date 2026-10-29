---
title: P0 VM and supervisor fail-closed integration boundary
status: Proposed
owner: Kernel and Services Working Group
last_updated: 2026-10-29
---

# ADR-039: P0 VM and supervisor fail-closed integration boundary

This is a partial security repair, not completion of BH-P0-VM-SM-INTEGRATION.

## VM authority and activation gate

`CAP_TYPE_VM_SPACE` is distinct from physical MEMORY. Native invocation first
validates the caller's CSpace capability type, then interprets object opcodes.
Process opcode 1 is reserved for VM CREATE; existing Process 4–7 and Bootstrap
operations retain their meanings. Bootstrap image entry declarations are unchanged.
No syscall number, syscall carrier, or ABI lock is modified.

Version 1 of `bharat/uapi/system/vmm.h` reserves a fixed-width 56-byte request.
It accepts no caller PID, space pointer, or physical address. Its fields are
copied through usercopy before validation. Unknown versions/sizes/reserved bits,
W+X, representational overflow, and unaligned mutations are rejected.

| Operation | Object authority | Required kernel rights | Current result after validation |
| --- | --- | --- | --- |
| CREATE | Self Process in caller CSpace | RESOURCE_ALLOC | K_ERR_UNSUPPORTED |
| DESTROY | VM_SPACE | VMM_MANAGE | K_ERR_UNSUPPORTED |
| MAP | VM_SPACE | MEMORY_MAP | K_ERR_UNSUPPORTED |
| UNMAP | VM_SPACE | MEMORY_UNMAP | K_ERR_UNSUPPORTED |
| PROTECT | VM_SPACE | VMM_MANAGE | K_ERR_UNSUPPORTED |
| QUERY | VM_SPACE | QUERY | K_ERR_UNSUPPORTED |

VM_SPACE grants contain only MAP, UNMAP, MANAGE, QUERY. Delegation is intentionally
unavailable until lifetime ownership is safe. The generic authority resolver maps
READ to QUERY, WRITE to VMM_MANAGE, EXECUTE to MEMORY_MAP. It no longer treats a
physical memory capability as a VM-space capability.

Activation requires a revoke-safe pin/release protocol spanning CSpace lookup
and object use, and authoritative backing extents/access permissions. Current
`cap_lookup_memory` reports an invented 4096-byte extent. No dispatcher operation
uses that value as backing authority. No new mapping database, global object
registry, Linux VM dependency, or fabricated QUERY response is introduced.
All profiles including MPU return truthful unsupported responses. Full CREATE,
MAP, QUERY, PROTECT, UNMAP and distributed DESTROY are still outstanding.

The bounded distributed destroy allocator now detects a full transaction pool
before indexing a transaction handle. Exhaustion quarantines the space as
POISONED and returns BUSY rather than using an uninitialized slot.

## Supervisor state and ownership

The supervisor and staging arrays belong to its single event loop. Internal
BEGIN validates version/count/session and starts a separate staging graph.
SERVICE validates order, IDs/names, duplicate identities and dependency shape.
COMMIT checks session/count and detects missing dependencies/cycles before
publishing the complete array and session. Failure discards staging while
preserving the active graph. Repeated COMMIT is rejected. Adoption preserves
process/incarnation values and begins at STARTING/SPAWNED, never RUNNING.
Internal staging functions are not authenticated wire entrypoints; production
RPC is disabled until launch identity and authority can be verified.

Health uses only `SM_OP_HEALTH_UPDATE` and the canonical request/response types.
The internal handler requires an independently authenticated PID matching the
registered process, boot session and incarnation; sequences must increase.
SPAWNED -> BOUND -> READY establishes readiness. Repeated SPAWNED is liveness,
not readiness. READY may become DEGRADED and recover to READY. FAILED invalidates
stability. STOPPED requires a stopping lifecycle. Supervisor monotonic time
supplies liveness, never the client's timestamp/deadline. Exact wire lengths
are checked before request fields. The internal API has an explicit PID argument
so a future adapter cannot silently substitute the claimed request PID.

Spawn errors and absent launchers propagate errors. Stop request acceptance
leaves STOPPING; only a later PM EXITED query permits terminal cleanup/restart.
A failed service retains process identity until exit is proved. Bounded restart
budgets and capped exponential delays prevent unbounded immediate retry. Critical
failure/degradation resets both the stability timer and achieved stability.
STABLE requires a committed graph and a continuous healthy window with current
liveness. Registration publishes the created SEND capability, never RECEIVE;
publication failure prevents the event loop. No manager READY marker is emitted.

## Missing production identity transport

The current RPC helper exposes a single received capability as reply SEND
endpoint, not independent authority, and no authenticated sender PID. Numeric
`capability_transfer`/`reply_endpoint` fields do not prove caller identity.
Supervisor RPC rejects all operations after decoding. It does not send to an
unverified numeric reply endpoint or install a fake resolver. Init returns
ENOSYS and retains lifecycle ownership rather than transmit a forged bootstrap
handle or overwrite genuine launch process IDs with zero. Internal stage tests
prove graph mechanics only; no secure handoff execution is claimed.

The generic runtime now requires nonzero process/session/incarnation startup
identity, maintains per-instance sequence, supplies the IPC version, propagates
send failure and leaves READY to service-specific initialization. Its startup
adapter does not yet have all these identities; it fails closed. This additive
internal context change is not a versioned public startup ABI migration.

Remaining blockers: authenticated IPC identity/authority and reply provenance,
trusted launch/incarnation startup data, stage launch-state/timing verification,
full VM lifetime/backing mechanisms, guest native VM lifecycle tests and secure
handoff/recovery execution. Host tests cannot satisfy those gates.
