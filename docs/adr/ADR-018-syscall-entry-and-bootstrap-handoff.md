# ADR-018: Normalize syscall entry and bound bootstrap handoff failure

## Status

Accepted

## Context

The x86_64 `SYSCALL` stub called the two-argument common syscall gate with only
the trap-frame argument.  The uninitialized trap metadata caused native calls
to fail before dispatch, so `services/init` entered user mode but could not
publish boot evidence.  In addition, headless and small-device packages do not
currently contain a separately launchable `servicemgr`; treating its absence as
an unbounded init failure made an otherwise healthy GP boot time out.

## Decision

Architecture syscall stubs enter the common gate through a fixed-signature C
bridge which creates complete, user-origin syscall metadata.  Native `write`
uses the implicit current-process authority for the bootstrap console path;
pointer data remains bounded and fault-safe copied by the handler.

The manifest-driven init service still attempts the versioned servicemgr
handoff.  If discovery reports that no supervisor is packaged, profiles without
strict core deadlines publish `HANDOFF_DEFERRED`, retain a degraded quiescent
bootstrap authority, and may report a stable service graph.  RT and safety
profiles remain fail-closed: rejection, timeout, malformed replies, and missing
supervisors are fatal handoff failures.

## Invariants

- Architecture entry assembly never supplies partially initialized common trap
  metadata.
- User pointers are not dereferenced before the existing fault-safe usercopy.
- A rejected or timed-out handoff is never converted to success.
- Only an absent supervisor may be deferred, and only for a non-strict profile.
- No kernel object or scheduler lock is held across supervisor discovery or IPC.
- Deferred mode is explicit degraded evidence, not a fabricated handoff ACK.

## Consequences

Five-architecture builds share the same metadata-driven gate and profile policy.
Packaging a real servicemgr remains the production path; strict profiles cannot
use the development fallback.
