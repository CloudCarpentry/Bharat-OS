---
title: Authenticated IPC receive envelope and migration proposal
status: Proposed
owner: Kernel and Services Working Group
last_updated: 2026-10-29
---

# Authenticated IPC envelope proposal

This is a design proposal for BH-P0-IPC-IDENTITY-001, not an implemented ABI,
an approved lock migration, or evidence that Task 01 has passed. The accompanying
fail-closed repairs do not expose authenticated receive metadata. ADR-039's
runtime activation gates remain in force.

## Trust boundary

The kernel obtains sender identity from the syscall context and its live CSpace,
after fault-safe copying of the send descriptor. It records the identity when it
accepts the message. A service receives a kernel-written envelope separately from
the sender-written service header. Repeating an envelope's bytes in a payload
never creates authenticated provenance.

The distinctions are mandatory:

| Field or resource | Meaning and source | Permitted use |
| --- | --- | --- |
| Sender PID | Kernel process identity at send acceptance | Identify the actual process; insufficient for administrator rights |
| Sender process object ID | Kernel process object identity | Fence process reuse only under a documented allocation/wrap contract |
| Sender CSpace ID and owner core | Kernel registered namespace identity | Trace namespace provenance; never a handle usable in the receiver's CSpace |
| Service authority capability | Independently checked, receiver-local capability, bound to object, operation rights and scope | Authorize only the granted operation |
| Reply capability | Receiver-local endpoint SEND capability delivered through the kernel | Send a correlated response; confers no service administration rights |
| Message ID | Sender-selected correlation value | Match request and reply; does not authenticate a sender or prevent replay alone |
| Boot session | Trusted bootstrap identity with explicit validity flag | Fence previous boots only after its provisioning contract exists |
| Service incarnation | Trusted launch identity with explicit validity flag | Fence previous service instances only after PM/bootstrap provisioning exists |

At this baseline, `bh_process` has `process_id` and `object_id`, and the CSpace
registry has a generated `cspace_id`. `process_create()` does not initialize
`bh_process.generation`; that field must not be attested as a valid incarnation.
There is no kernel-backed boot-session or service-incarnation provisioning path.
Zero values with validity flags cleared must describe unavailable identities.
User argv, request headers, health payloads and startup claims cannot fill them.

## Candidate fixed-width envelope

The following 96-byte shape is illustrative. It is not installed in a public
header. A reviewed implementation must assert every field offset and total size
on all five architectures, with explicit ABI alignment and no implicit padding.
Unknown versions, sizes, flags and nonzero reserved fields are rejected.

```c
struct bh_ipc_receive_envelope_v1 {
    uint32_t version;                 /* offset 0 */
    uint32_t struct_size;             /* 4 */
    uint64_t valid_fields;            /* 8 */
    uint64_t sender_pid;              /* 16 */
    uint64_t sender_process_object_id; /* 24 */
    uint64_t boot_session_id;         /* 32 */
    uint64_t service_incarnation_id;  /* 40 */
    uint32_t sender_cspace_id;        /* 48 */
    uint32_t sender_owner_core;       /* 52 */
    uint32_t authority_cap;           /* 56, receiver-local */
    uint32_t reply_send_cap;          /* 60, receiver-local */
    uint32_t authority_object_type;   /* 64 */
    uint32_t reserved0;               /* 68, zero */
    uint64_t authority_rights;        /* 72 */
    uint64_t message_id;              /* 80, correlation only */
    uint64_t delivery_generation;     /* 88 */
};
```

Validity bits separately describe PID, process object, CSpace, boot session,
incarnation, authority and reply availability. A validity bit is set only after
the associated kernel check succeeds. A service requires all bits needed for
its operation, rather than treating an all-zero identity as privileged.
Capability rights in metadata are descriptive: the live receiver-local handle
must remain valid through use under the revocation protocol.

The current transport transfers **one** capability. An initial metadata-only
extension must preserve that constraint: exactly one of `authority_cap` and
`reply_send_cap` can be nonzero, with an explicit checked transfer role. An RPC
needing both resources must remain unsupported. Supporting both requires an
explicitly reviewed later transfer-vector version or a kernel-minted,
service-scoped endpoint badge. Neither is implemented or implicitly authorized
by this proposal. A reply SEND capability cannot be relabeled as authority.

The generic `kernel_cap_authority_resolver()` currently ignores
`expected_object_id` and `required_scope`, and maps SERVICE to an unconstrained
kernel object type. It cannot provide the proposed service-authority guarantee.
The future authority path must enforce type, object identity, rights, scope,
generation, revocation epoch and namespace without such fallback behavior.

## ABI migration

The stable receive carrier is a 40-byte unversioned structure; it has no reserved
metadata selector. Its existing output capability pointer must retain its
meaning. Appending fields and blindly copying a larger carrier would overread
old callers. Reinterpreting that output pointer as an envelope would corrupt
old callers. Send's `reserved0` is not an undocumented protocol selector.

A candidate additive design keeps the syscall number and argument-0 pointer,
and adds an explicit optional version/size selector in an unused register:
selector zero selects the exact legacy carrier; a recognized nonzero selector
selects a new versioned carrier and separate envelope output. This changes
argument semantics and therefore requires an intentional manifest/lock review;
it must not be implemented solely in a hand-written header. Another candidate
uses versioned endpoint object operations on `CAPABILITY_INVOKE`, with checked
endpoint rights and no new syscall number. The selected approach needs one
authoritative contract, not parallel dispatch definitions.

Before activation:

1. Specify the carrier, selector, fixed-width address representation, direction,
   exact sizes, rights, transfer roles and failure semantics in
   `interface/contracts/abi/native_syscalls.json` and associated UAPI authority.
2. Review compatibility against `native_syscalls.lock.json`; update the lock
   only for the intentional approved expansion. Generate outputs under build/.
3. Validate 32-bit address representability before casts or usercopy. Copy each
   descriptor once, then validate embedded handles from the kernel copy.
4. Provide strict old/new caller tests, size/offset assertions, usercopy failure
   tests and `python3 tools/abi/syscall_abi.py --check` evidence.
5. Define timeout units explicitly. Legacy endpoint timeouts are scheduler
   ticks while `TIME_GET(BH_CLOCK_MONOTONIC)` returns nanoseconds. New operations
   should use an absolute monotonic-nanosecond deadline with overflow checks;
   there must be no silent conversion of the legacy field.

## Ownership, lifetime and failure behavior

Endpoint queues, delivery records and capability slots have an explicit owner
core. Cross-core transfers carry fixed-width locators containing CSpace identity,
slot, generation and revocation epoch; no source-table pointer enters a queued
message. Resolving a locator is not itself a lifetime pin. Registry unpublish,
locator resolution and the entire local use interval must share an owner-local
serialization or pin protocol. Unsupported cross-owner delivery is denied until
a bounded owner-core protocol exists.

The future delivery lifecycle is PREPARED -> QUEUED -> INSTALLING -> DELIVERED,
with CANCELLED/REVOKED terminal states. A receiver may claim a record once.
Transfer slots are reserved before publication; partial installation rolls back
in reverse order. Payload and metadata copyout failure releases every newly
installed receiver-local capability. No endpoint or CSpace lock is held while
waiting for remote acknowledgement. Reacquisition must revalidate record,
endpoint, CSpace, capability generation and epoch before commit.

Revoke denies new installations and waits boundedly or defers reclamation of
existing pins. Timeout, endpoint destruction, sender teardown and receiver
teardown cancel queued deliveries and release reserved resources. A late reply
cannot resurrect a cancelled call or target a reused endpoint generation.
Uncertain remote rollback quarantines the delivery/object; reporting success
or reusing its slot is forbidden. Retry is bounded, addresses only missing
participants and uses transaction IDs/generations for idempotence.

Public capability release and endpoint destruction/cancellation are prerequisites
for an allocating call helper. Dropping numeric IDs without object teardown does
not prove that the endpoint pool is reclaimed. A reply must match reply flag,
message ID, service/interface/version/opcode and delivery generation before any
caller response buffer is modified. Replay protection also needs a trusted
session/incarnation and service operation sequence, not just message correlation.

## Acceptance still required

- Kernel-backed positive authenticated calls to VM Manager and supervisor with
  independently scoped operation authority and a separate genuine reply path.
- Denial of forged PID, service identity, authority/reply fields, wrong service,
  wrong object/scope/rights, stale or revoked capability and replayed incarnation.
- Trusted bootstrap/PM provisioning of boot session and service incarnation;
  authenticated launch-state checks before handoff or privileged mutation.
- Revoke-safe endpoint/CSpace/object use; teardown, cancellation and copyout
  rollback without leaks, UAF, ABA, double installation or double free.
- At least 10,000 real send/receive/timeout/failure cycles with resource counters,
  concurrency/revoke/destroy races and allocation/queue/transaction saturation.
- Bounded cross-core ACK/NACK, timeout, delayed ACK, duplicate/replay and partial
  failure tests under the actual transport; no mocked authentication boundary.
- A trace containing kernel-attested metadata and matching checked receiver-local
  handles; never a trace of sender-serialized identity presented as authentication.
- Full host build/test accounting, ABI/lints, five-architecture layout/build/CORE
  smoke, and separate guest authentication/service integration evidence. CORE
  STABLE alone does not satisfy runtime integration.

Until these gates pass, Task 01 remains incomplete and privileged service wire
operations must fail closed. This proposal makes no VM lifecycle, supervisor
handoff, recovery, SMP qualification or production-readiness claim.
