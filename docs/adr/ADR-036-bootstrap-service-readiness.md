---
title: Real CORE bootstrap through capability invocation and child-specific IPC
status: Proposed
owner: Kernel and Services Working Group
last_updated: 2026-10-09
---

# ADR-036: Real bootstrap launch and readiness

## Context and decision

BOOT-FLOW-P0-001 requires real namesvc and process_manager execution. The
developer baseline accepted both launches without creating processes, had no
readiness receiver, and could replace required failure with degraded handoff.

Use the existing native CAPABILITY_INVOKE, endpoint send/receive, and monotonic
time syscalls. Syscall numbers and the compatibility lock remain unchanged.
The object-operation payload authority is
`interface/include/bharat/uapi/bootstrap/service_launch.h`; version 1 uses
fixed-width fields with size and offset assertions on all five architectures.

BOOTSTRAP operations are LAUNCH (1), PROBE (2), CREATE_IMAGE (3). PROCESS
operations are START (4), QUERY (5), REAP (6), TERMINATE (7). These are object
operation codes, not syscall numbers. Syscall-only user addresses are copied
through fault-safe usercopy and checked for overflow/truncation. No pointers
cross IPC. CREATE_IMAGE is bounded to 1 MiB, uses page-sized copies and the
existing ELF loader, and returns a stopped thread with real IDs. The native
process-manager adapter uses these mechanisms; unsupported memory profiles
fail explicitly.

Bootstrap authority must be typed, live, have LAUNCH, and refer to the current
owner-core process. Creating another launcher also requires BIND and DELEGATE.
PROCESS operations require MANAGE and reject self/remote-core mutation.
Exported process handles derive from loader self-process roots; destruction
revokes those roots before slot reuse. All new mutable objects are core-local.
The kernel stores no service graph or READY state.

Each child owns its service endpoint and receives a dedicated readiness SEND
capability. Handles are derived into destination CSpaces with attenuated
rights, never copied as parent integers. The parent keeps the readiness SEND
root alive because its revocation would revoke the child sender. Creating the
first discovery server requires BIND; other children receive discovery SEND.

Events are 16 bytes: version, type, service ID, status. Init validates the
dedicated receiver, version/ID/order/status, accepts BOUND before READY,
reevaluates dependencies after each event, and uses monotonic nanosecond
deadlines. Launch success only establishes SPAWNED. Process_manager launches
after namesvc's validated READY. Init retains lifecycle authority when no
supervisor is selected.

The explicit `BHARAT_INIT_CORE_BOOTSTRAP_ONLY` development target policy
packages/selects this two-service P0 graph. All five canonical headless smoke
contracts require real service execution and CORE readiness. This does not
qualify the full production graph.

## Boot-module compatibility

Packages concatenate existing little-endian 128-byte version `0x100`
containers: one init root (kind 1), followed by service ELFs (kind 3). Existing
magic is `0xB4A2D1A5`, payload offset is 128, payload length is nonzero/bounded,
and the terminated name is 1–31 bytes. Normalization expands descriptors within
`BHARAT_BOOT_MAX_MODULES`; malformed/truncated headers, overflow, invalid
versions/names, and appended root kinds fail. Existing single-container and
raw named-root paths remain compatible. Digest verification is not added.

## Architecture boundaries

x86_64 processes own private lower-half page-table hierarchies and inherit only
supervisor leaves. User ELFs link at `0x40000000`, outside low identity kernel
text. Generated configuration precedes legacy tracked configuration includes.
ARM early boot libraries compile with strict alignment. ARM64 uses the ELR
already advanced by SVC hardware. ARM32 restores banked user SP/LR after
syscall scheduling and keeps AAPCS alignment across its post-switch C call.
RISC-V64 provides the supervisor high aliases expected by the existing QEMU
bootstrap translation window. IPC payloads pass through kernel buffers and
fault-safe usercopy, including transferred-cap cleanup after failed copyout.

## Failure and scope limits

Required failures stop graph progression and prevent healthy handoff markers.
Rollback requests termination/deferred reap; failed compensation reports FAILED
and quarantine. The existing endpoint backend lacks blocked-wait cancellation
and endpoint reclamation. Blocked termination returns unsupported; failed
launch/rollback objects may remain quarantined in bounded pools with retained
authority. Implementing general cancellation is outside this recovery.

General service RPC, supervisor handoff, restarts, SMP launch/migration,
non-QEMU platforms, MPU-only targets, and the full graph are not qualified.
Some existing service RPCs exceed the default endpoint payload limit. Readiness
proves actual execution, endpoint acquisition, initialization and native backend
installation, not complete OS functionality or secure boot.

## Validation

Run `bash tools/testing/test_bootstrap_recovery.sh` after the x86_64 build,
the five canonical build/smoke commands in root `AGENTS.md`, and
`python3 tools/run_qemu_matrix.py --headless --smoke --all-arch`.
Focused checks cover event progression/dependencies, missing readiness,
required failures, status translation, usercopy/cap cleanup, x86 isolation and
allocation rollback, malformed containers, process-manager rollback and 1,000
lifecycle cycles, component selection, and parser rejection of fatal/missing
service evidence. QEMU capability selftests cover typed rights, attenuation,
stale handles and recursive revocation.

This proposed ADR records the implemented recovery boundary for maintainer
review; it does not claim a broader accepted ABI migration or release status.
