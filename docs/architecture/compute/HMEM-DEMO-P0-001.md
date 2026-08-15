---
title: "HMEM-DEMO-P0-001: real userspace HMEM/Tensor vertical path"
status: Proposed
owner: Compute and Runtime Working Groups
last_updated: 2026-08-13
tags:
  - architecture
  - compute
  - userspace
  - validation
---

# HMEM-DEMO-P0-001: real userspace HMEM/Tensor vertical path

## 1. Objective and completion claim

Launch `bench_hmem_tensor` through the production-shaped lifecycle:

```text
servicemgr
  -> process_manager
  -> vm_manager
  -> kernel process
  -> process-owned CSpace
  -> initial user thread
  -> architecture user entry
  -> HMEM/Tensor Native UAPI
  -> process exit notification
  -> process_manager wait/reap
```

This task is complete only when the chain executes in a QEMU guest. A host
executable, direct call to the benchmark's `main`, kernel-only benchmark, mock
authority adapter, synthesized console marker, or service-local state-table test
is useful supporting evidence but is **not** evidence for the completion claim.

## 2. Current baseline and gap

The repository currently has three separate pieces of evidence:

1. `bench_hmem_tensor` exercises the SDK HMEM and Tensor APIs as a host
   executable.
2. the QEMU benchmark exercises the kernel HMEM mechanism without entering the
   SDK benchmark as a user process; and
3. process/VM host tests exercise service transactions through injected
   authority adapters.

They do not form one authority-preserving path. In particular, the standalone
`process_manager` intentionally refuses to advertise itself without a real
kernel authority adapter. The task must close that boundary rather than replace
the fail-closed adapter with successful fake identifiers.

## 3. Architecture invariants

### 3.1 Responsibility placement

* `servicemgr` owns launch/supervision policy and requests a launch by immutable
  executable identity.
* `process_manager` owns the service-local lifecycle transaction and is the only
  mutator of its bounded process records.
* `vm_manager` owns mapping policy and invokes capability-authorized kernel VM
  mechanisms. It does not own kernel address spaces.
* the kernel owns the process, address space, CSpace, threads, and scheduling
  mechanisms. A process CSpace is never replaced by a CPU-global table.
* the SDK owns Tensor metadata. The kernel sees only HMEM descriptors, handles,
  rights, mappings, and synchronization operations.

### 3.2 Authority and trust boundary

Every service-to-kernel operation uses a generated Native ABI operation and a
capability supplied through the caller's real CSpace. Required validation is:

| Operation | Object authority | Minimum checks |
| --- | --- | --- |
| create process | process-factory/root job | type, create right, scope, liveness, owner |
| create/map space | process and VM-space | type, map right, range, generation, owner |
| load image | executable and VM-space | executable read/execute, VM write/map, immutable image identity |
| create/start thread | process/thread | type, execute right, entry and stack in user mappings |
| HMEM create/map | HMEM factory/object | descriptor, HMEM rights, generation, process CSpace ownership |
| exit/reap | process | wait/reap right, terminal state, generation, parent/supervisor scope |

No service passes kernel pointers, address-space pointers, CSpace pointers,
thread pointers, callbacks, or architecture-sized `long` fields across IPC.
Wire-visible requests use fixed-width values and exact version/size checks.

### 3.3 Ownership, synchronization, and lifecycle

The kernel process object and its address space, CSpace, and initial thread are
owned by the core that creates the process. Remote requests carry object ID and
generation and are routed to that owner; they do not directly mutate foreign
objects. The service transaction holds no VM, CSpace, process, or scheduler lock
while waiting for an acknowledgement.

The required lifecycle is:

```text
RESERVED -> CREATED -> IMAGE_READY -> RUNNABLE -> RUNNING
        -> EXITED -> REAPING -> FREE
```

Any failure before publication compensates in reverse order. Destruction first
makes the object `DYING`, rejects new map/thread/HMEM mutations, removes runnable
threads, revokes process capabilities, destroys mappings, and finally returns
the generation-bearing slot to its owner. If compensation cannot safely
complete, the object is quarantined and remains unavailable for reuse.

Exit delivery uses a bounded owner-to-service event queue. Events contain ABI
version, structure size, process ID, process generation/incarnation, exit code,
reason, and event sequence. Duplicate events are idempotent; queue-full returns
explicit backpressure; stale generations are rejected. Blocking wait is not
required for P0: bounded non-blocking wait followed by reap is sufficient.

## 4. Implementation slices

### Slice A: capability-mediated lifecycle transport

1. Add missing process/address-space/image/thread/start/exit/reap operations to
   `interface/contracts/abi/native_syscalls.json`; regenerate dispatch artifacts
   and intentionally update the ABI lock.
2. Implement kernel handlers using the canonical process, VM, CSpace, image
   loader, and scheduler mechanisms. Do not create a parallel object registry.
3. Add SDK/service wrappers which use generated syscall identifiers only.
4. Install the production `process_manager` adapter during userspace startup
   only when all required authority capabilities are present.

The adapter must remain fail closed on MMU-Lite and MPU if the image cannot be
represented by their authoritative protection backends. An unsupported backend
returns the canonical unsupported status and publishes no process handle.

### Slice B: service orchestration and executable authority

1. Package `bench_hmem_tensor` as an immutable user ELF module with a stable
   executable identity in the selected delivery profile.
2. Make `servicemgr` request the v1 spawn transaction rather than the legacy
   create/start table.
3. Resolve executable identity through a loader/package capability; remove the
   borrowed image-pointer registration from the production path.
4. Have `process_manager` request VM realization, seed only the minimum HMEM,
   console, exit, and self capabilities, then start the initial thread.

### Slice C: user execution and HMEM/Tensor proof

1. Enter the benchmark through the architecture user-entry trampoline with a
   mapped user stack and read-only startup block.
2. Route SDK HMEM operations through the Native UAPI when running in a guest;
   retain the host backend only for host SDK tests.
3. Keep Tensor views entirely in userspace and verify that their backing HMEM
   handle is process-owned and generation-valid.
4. Exit through the Native process-exit operation. Deliver the terminal event,
   observe it through non-blocking wait, reap the process, and prove that the
   process, thread, address space, CSpace entries, and HMEM object were released.

## 5. Failure and negative-path requirements

Focused tests must inject failure after each allocation/publication stage and
prove reverse-order cleanup. They must also cover:

* missing or wrong-type executable, process, VM, thread, and HMEM capabilities;
* insufficient rights, wrong scope/owner, revoked and stale generations;
* malformed ELF, entry outside executable mappings, invalid stack, and mapping
  overlap/overflow;
* process/thread/CSpace/VM/HMEM exhaustion and event-queue saturation;
* duplicate spawn/exit/reap requests and stale exit replay;
* user fault before HMEM creation, during a mapped HMEM lifetime, and during
  exit; and
* unsupported MMU-Lite/MPU layouts without leaked or published objects.

## 6. Guest evidence contract

The benchmark emits its existing `BH_BENCH` records. The lifecycle path adds
records at the point where each component actually completes work:

```text
BH_HMEM_DEMO:SERVICEMGR_LAUNCH_REQUESTED
BH_HMEM_DEMO:PROCESS_CREATED=<fixed-width id:generation>
BH_HMEM_DEMO:VM_REALIZED
BH_HMEM_DEMO:CSPACE_SEEDED
BH_HMEM_DEMO:USER_THREAD_STARTED
BH_BENCH:TEST=SDK_HMEM_TENSOR
BH_BENCH:RESULT=PASS
BH_HMEM_DEMO:PROCESS_EXITED=<exit code>
BH_HMEM_DEMO:PROCESS_REAPED
BH_HMEM_DEMO:RESOURCE_BASELINE_RESTORED
```

The QEMU test parser requires all records in order, exactly one benchmark
result, exit code zero, a matching process generation, and restored resource
counters. The parser must reject kernel-synthesized user/benchmark records and
truncated runs.

## 7. Validation and completion matrix

Focused validation includes the syscall ABI checker, transaction rollback and
capability-negative host tests, guest log parser tests, and one guest execution
per affected ISA. Completion additionally requires the repository's layer and
CMake dependency linters, all five required target smoke builds, and the
all-architecture QEMU matrix.

P0 may truthfully report per-target `UNSUPPORTED` for an image layout that the
MMU-Lite or MPU backend cannot represent, but that does not satisfy the five
target completion claim. The delivery/profile documentation must distinguish a
supported fail-closed result from an end-to-end passing target.

## 8. Documentation impact

When each slice lands, update the Native ABI authority and lock, the process and
VM lifecycle contracts, the HMEM/Tensor boundary ADR if semantics change, the
selected delivery target, the benchmark instructions, and the architecture
contract lookup map. Do not change a maturity table to “implemented” until the
ordered guest evidence and cleanup proof pass on the claimed target.
