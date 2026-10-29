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


## Task 01 follow-up: pending-transfer and legacy IPC quarantine

BH-P0-IPC-IDENTITY-001 remains incomplete. The follow-up security repair does
not implement authenticated receive metadata or enable service RPC. See
[pending-transfer verification](../architecture/p0-ipc-identity-verification.md)
and the [unimplemented envelope proposal](../architecture/p0-ipc-envelope-proposal.md).

Queued endpoint messages retain a fixed-width capability locator and requested
rights instead of a sender CSpace pointer. Local capture/install revalidate LIVE,
generation, revocation epoch, DELEGATE, type policy and rights. Both CSpaces must
belong to the executing core. Cross-owner installation fails closed; no remote
CSpace is dereferenced and no endpoint lock spans capability installation.

The owner-local registry lifetime lock excludes interrupts while resolving and
using source storage. Lock order is registry then the existing ordered CSpace
locks. Creation reserves a slot/generation before allocation, publishes only
initialized storage and rolls back reservation on failure. Teardown unpublishes
before free. Seven slot bits encode all 64 nonzero slot IDs without aliasing the
owner bits. Finite encoded generations retire a registry slot before identity
wrap. This is an internal locator-encoding fix, not a syscall-number or public
carrier-layout change.

Endpoint state is EMPTY -> QUEUED -> CLAIMED -> EMPTY. Installation runs outside
the endpoint lock. Receiver saturation restores QUEUED and wakes a receiver;
stale/revoked/unpublished sources discard the message and wake a sender. Fixed
scheduler-tick deadlines saturate on addition overflow and are not reset by
spurious wakeups; endpoint and transfer rights are checked again after wake.

The legacy data-only IPC wrapper declines received capabilities through the
kernel NULL output sink and strips serialized authority/reply tokens. Unknown
flags and unsupported transfer claims fail. Native syscall errors are decoded
using the public BH_ERR_* constants. High-level call returns UNSUPPORTED before
allocating an unreclaimable reply endpoint, sending work or modifying outputs;
its invalid nanosecond/tick arithmetic is removed. This intentionally disables
legacy RPC clients, including namesvc lookup/register helpers. Plain raw endpoint
IPC and CORE bootstrap remain separate paths.

VM wire dispatch consumes and rejects unauthenticated requests without invoking
its internal metadata handlers, generic authority resolver or numeric reply
routing. Exact request sizes and known header/interface versions are checked.
Supervisor wire authorization remains denied. No false VM success or READY marker
is introduced. The V1 header remains 56 bytes with compile-time size/offset checks.

This repair does not establish endpoint allocation concurrency or reclamation,
full installed-descendant teardown, generic capability object pinning, trusted
launch/session/incarnation, independent scoped service authority, cross-owner IPC
or authenticated guest RPC. The global endpoint pool still has no close path.
The transfer regression reuses endpoint objects; its balanced CSpace/transferred
slot counters are not evidence of endpoint-pool reclamation.
