# BH-P0-VM-SM-INTEGRATION baseline

Base: `a55497a5` (origin/developer, fetched 2026-10-29). Work is isolated
in `feature/p0-vm-capability-servicemgr`; the original checkout is untouched.
The remote exposes six branches, with no VM/handoff/servicemgr feature branch.

Verified existing mechanisms: generation-encoded CSpace handles, typed rights
validation, native Process and Bootstrap invocation, canonical address-space
regions, distributed VM mutation gates, and real bootstrap launch identities.
The recent IPC request/reply changes reserve the single transferred capability
for reply SEND authority. A header capability number is not sender identity.

Missing: VM-space capability type/UAPI/dispatch, backing-memory extent metadata
(`cap_lookup_memory` currently invents a 4096-byte extent), object pinning across
capability lookup/revoke/destroy, authenticated service RPC sender identity, and
trusted startup process/incarnation identity for the generic service runtime.
Distributed VM destruction also uses an uninitialized transaction handle when
the bounded transaction pool is full.

Observed baseline failure: `test_servicemgr` compilation references removed
`sm_req_heartbeat_t`, `sm_resp_heartbeat_t`, `SM_OP_HEARTBEAT`, and
`SM_OP_SIGNAL_READY`. Existing handoff BEGIN clears the active graph; adoption
marks any nonzero process ID RUNNING; launch/stop failures can report success.
The stable-window timer cannot reset. Init discards actual launch process IDs.

Plan: establish a canonical fail-closed VM contract and type-first routing;
repair health/lifecycle and staged graph validation with focused regressions;
reject unauthenticated RPC rather than installing a test authorization backend;
preserve real identities and reject narrowing; run host, ABI, static and
architecture gates and record runtime evidence separately. No readiness or
VM success may be inferred from mocks or unsupported mechanisms.

Environment: pip-installed CMake/Ninja allow a native GCC host configuration;
the prescribed Clang preset cannot run unchanged. System package installation
is denied (unprivileged actor). QEMU/cross toolchains are initially absent.
GitHub CLI reports its configured token invalid; PR publication is unverified.
