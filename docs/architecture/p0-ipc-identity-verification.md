# BH-P0-IPC-IDENTITY-001 security repair verification

Date: 2026-10-29. Baseline: freshly fetched `developer`,
`a8323c1f5ebdf4d14212922994b5ef64091341b0` (PR #1104).
Branch: `codex/bh-p0-ipc-identity-001`.

> [!NOTE]
> **Progress Update (BH-IPC-P0-002, October 2026)**:
> Blocker 1 below ("Implement the reviewed, versioned receive envelope through the ABI authority...") has been **implemented and verified** on branch `feature/ipc-authenticated-receive-envelope` (commit `ad376193`).
> See the dedicated architecture document [docs/architecture/ipc-receive-envelope-architecture.md](ipc-receive-envelope-architecture.md) and review [docs/reviews/ipc-p0-002-authenticated-receive-envelope-review.md](../reviews/ipc-p0-002-authenticated-receive-envelope-review.md).
> Blockers 2–7 remain active production hardening milestones.

**INCOMPLETE — security repair only; draft PR; no automatic merge.**
Authenticated IPC origin, independent scoped service authority, verified reply
provenance and guest service integration are not implemented. The envelope is
an explicitly unimplemented proposal. No ABI manifest/lock migration was made.
Do not promote this change as Task 01 completion or P0 production readiness.

## Implemented scope and compatibility

- Queued transfers use a pointer-free CSpace/slot/generation/epoch locator.
  Owner-local registry lifetime serialization spans source resolution and local
  delegation; no endpoint lock is held during installation. Cross-owner
  installation fails closed. LIVE, DELEGATE, type policy and rights attenuation
  are checked again at installation. Stale sources are discarded; receiver-table
  exhaustion preserves the queued message and wakes another receiver.
- CSpace creation reserves identity before allocation/reentry and publishes only
  initialized storage. Allocation failure releases reservation. The slot-64
  encoding alias is repaired, and registry slots retire before encoded generation
  wrap. Teardown unpublishes before source storage is freed.
- Endpoint waits use one overflow-saturating scheduler-tick deadline across
  wakeups and revalidate endpoint/transfer authority after blocking.
- Data-only libipc receive uses the kernel NULL cap sink, rejects unsupported
  transfer/unknown flags, strips serialized authority/reply tokens and preserves
  outputs on decode failure. Native status mapping uses public BH_ERR constants.
- High-level `bharat_ipc_call[_ex]` returns UNSUPPORTED before allocation/send or
  output changes. This intentionally disables existing high-level RPC clients,
  including namesvc lookup/register helpers. Their previous paths could not
  authenticate replies or reclaim endpoint/capability resources. Mixed
  TIME_GET-nanosecond/scheduler-tick calculations are removed; data send/receive
  keep their documented scheduler-tick units.
- VM wire dispatch validates versions, opcodes and exact lengths, then denies
  without invoking metadata-only handlers, authorizing a serialized number or
  replying through a serialized endpoint. Supervisor denial remains intact.
  Its negative fixture injects raw malicious wire bytes so sender hardening
  does not remove receiver-denial coverage.
- The existing V1 wire header stays 56 bytes; size and critical offsets have C/C++
  static assertions. No syscall number or generated syscall table changed.

Ownership and failure behavior are detailed in ADR-039's follow-up section.

## Executed commands and results

Every shell sources `/workspace/bharat-tools/activate.sh`. Clang/LLVM 19,
CMake 3.31.6 and QEMU 10.0.13 are installed outside the repository. Evidence is
under `/workspace/work/bh-p0-ipc-001`; generated build files remain ignored.
The projectless output directory is not writable from this cloud executor
(`/codex` creation is denied); raw logs remain in the execution workspace.
The committed verification report is the reviewable evidence summary.

| Gate / command | Result |
| --- | --- |
| `cmake --preset host-test` | PASS |
| Focused build and CTest commands below | PASS, 8/8 |
| Transfer target with ASan/UBSan commands below | PASS, 1/1; no sanitizer findings |
| `cmake --build --preset host-test --target bharat_tests/host/all -j2` | FAIL: pre-existing undeclared `bharat_msg_header_encode` in TLB transport fixture |
| `ninja -C build/host-test -k 0 bharat_tests/host/all -j2` | FAIL: 218 failed compile/link commands in the diagnostic attempt; no assertions or targets suppressed |
| `ctest --test-dir build/host-test --output-on-failure -j2` | FAIL: 119 discovered, 41 passed, 2 assertion failures, 76 not run because executables are missing |
| `python3 tools/abi/syscall_abi.py --check` | PASS, ABI manifest/lock unchanged |
| `python3 tools/lint/check_cmake_dependencies.py` | PASS, zero violations |
| `python3 tools/lint/check_layer_references.py` | Exit 0, with 11 reported violations and 101 known debt entries; not a zero-violation result |
| `python3 tools/agents/verify.py --changed-files <exact inventory below>` | PASS: agent config, layer command, display tiers, ABI |
| `python3 tools/lint/check_no_recursive_memops.py` | PASS, all five final kernel ELFs |
| `git -c core.whitespace=cr-at-eol diff --check` | PASS; preserves existing CRLF sources |
| `python3 tools/run_qemu_matrix.py --help` | PASS; runner supports all five targets |
| `python3 tools/run_qemu_matrix.py --headless --smoke --all-arch --json-report /workspace/work/bh-p0-ipc-001/qemu-matrix.json` | PASS, 5/5 build/link/CORE boot |
| Five individual `./tools/build.sh all --target-yaml ... --smoke` gates below | PASS, all five independently executed |
| Separate authenticated guest VM/supervisor integration | BLOCKED: missing authenticated transport, authority/reply provisioning and lifecycle mechanisms |

Full-suite failures are `host_test_capability_ownership` (distributed-delegation
assertion) and `host_test_sched_diag` (runtime diagnostic-level assertion).
The capability fixture received only platform IRQ stubs to link the real new
registry lifetime lock; its distributed assertion was preserved and still fails.
The full CTest command ran without a timeout override or stop-on-first-failure.

Focused commands:

```bash
cmake --build --preset host-test --target host_test_ipc_wire test_ipc_transfer_lifetime test_vm_ipc_boundary test_servicemgr host_test_servicemgr_lifecycle host_test_servicemgr_harden test_vm_capability test_init_readiness -j2
ctest --test-dir build/host-test -R '^(host_test_ipc_wire|host_test_ipc_transfer_lifetime|host_test_vm_ipc_boundary|host_test_servicemgr_lifecycle|host_test_servicemgr_harden|host_test_init_readiness|host_test_vm_capability|host_test_servicemgr)$' --output-on-failure
cmake --preset host-test -B /workspace/work/bh-p0-ipc-001/host-sanitized -DBHARAT_IPC_TRANSFER_SANITIZERS=ON
cmake --build /workspace/work/bh-p0-ipc-001/host-sanitized --target test_ipc_transfer_lifetime -j2
ctest --test-dir /workspace/work/bh-p0-ipc-001/host-sanitized -R '^host_test_ipc_transfer_lifetime$' --output-on-failure
```

The lifetime harness links actual endpoint, CSpace, capability policy,
revocation and allocator implementations. Machine, memory-allocation and scheduler
plumbing are substituted. It executes 12,000 local transfer/revoke/destroy/recreate
cycles, receiver saturation/retry, parked receiver wake, stale epochs, attenuated
rights, foreign-owner denial, slot-64 encoding, identity retirement, allocator
reentry/OOM, repeated wake deadline exhaustion, overflow saturation and revocation
after blocking. Transferred slots and CSpace allocations balance after every
cycle. This is deterministic interleaving coverage, not parallel SMP qualification.
Endpoints are reused; endpoint reclamation and authenticated origin are not proved.
Libipc and VM denial tests use transport fixtures and never mock authentication
into existence. The 10,000 RPC refusals allocate no resources but are not successful
RPC cycles or acceptance evidence for authenticated replies.

An independent agent reviewed the final trust boundary, lifecycle and source diff,
identified wakeup/status/identity-wrap/reservation issues that were fixed, and
independently ran the three focused IPC/VM tests (3/3 PASS). It recommended draft
partial delivery only, not Task 01 completion or merge.

## Five-architecture matrix

| Target | Build/link | CORE boot | Authenticated IPC / VM / supervisor integration |
| --- | --- | --- | --- |
| x86_64 desktop headless | PASS | PASS, 17/17 markers | BLOCKED, not implemented |
| ARM64 desktop headless | PASS | PASS, 17/17 markers | BLOCKED, not implemented |
| RISC-V64 desktop headless | PASS | PASS, 17/17 markers | BLOCKED, not implemented |
| ARM32 MMU-Lite headless | PASS | PASS, 17/17 markers | BLOCKED, not implemented |
| RISC-V32 MMU-Lite headless | PASS | PASS, 17/17 markers | BLOCKED, not implemented |

Each CORE smoke target selects `BHARAT_INIT_CORE_BOOTSTRAP_ONLY`. Namesvc and PM
bootstrap/readiness markers are not supervisor handoff or native VM evidence.
The optional VM launch reports `SPAWN_CALL_FAILED`/FAILED rather than success
when its legacy fabricated-capability call is rejected. No READY bypass is added.
MMU-Full, MMU-Lite and MPU/PMP are not claimed to offer identical functionality.
Physical boards, MPU hardware, RT latency and full SMP runtime are unqualified.

Required individual commands:

```bash
./tools/build.sh all --target-yaml delivery/targets/qemu/x86_64_desktop_headless.yaml --smoke
./tools/build.sh all --target-yaml delivery/targets/qemu/arm64_desktop_headless.yaml --smoke
./tools/build.sh all --target-yaml delivery/targets/qemu/riscv64_desktop_headless.yaml --smoke
./tools/build.sh all --target-yaml delivery/targets/qemu/arm32_mmu_lite_headless.yaml --smoke
./tools/build.sh all --target-yaml delivery/targets/qemu/riscv32_mmu_lite_headless.yaml --smoke
```

## Remaining acceptance blockers

1. **[RESOLVED in BH-IPC-P0-002]** Implement the reviewed, versioned receive envelope through the ABI authority:
   - **UAPI Contract**: 96-byte `bh_ipc_receive_envelope_v1_t` in `interface/include/bharat/uapi/ipc/envelope.h` with compile-time assertions on size and offsets.
   - **Syscall ABI**: 64-byte `bharat_sys_endpoint_receive_v1_args_t` in `syscall_args.h`; Syscall 9 selector migration in `native_syscalls.json` and locked in `native_syscalls.lock.json`.
   - **Kernel Provenance Attestation**: `sender_pid`, `sender_process_object_id`, `sender_cspace_id`, `sender_owner_core` captured at send acceptance and attested in envelope without fabricating unprovisioned fields.
   - **Single-Cap Role Mapping**: Transferred `CAP_TYPE_ENDPOINT` with `SEND` right mapped to `reply_send_cap` (`BH_IPC_ENVELOPE_F_REPLY_SEND_CAP`); other capability types mapped to `authority_cap`.
   - **Fault-Safe Rollback**: Immediate capability revocation on usercopy fault.
   - **Verification**: Host tests (`test_uapi_abi`, `test_ipc_transfer_lifetime`, `test_endpoint_usercopy`) and 5-target QEMU matrix (`ktest_ipc_envelope_run PASS` across x86_64, arm64, riscv64, arm32, riscv32).
2. Provide trusted boot-session/process-incarnation/service identity from PM and
   bootstrap. An uninitialized process generation, startup argv or request PID
   cannot establish it. Authenticated privileged VM/supervisor RPC stays denied.
3. Implement endpoint owner-core allocation/lifetime, close/cancel, release,
   copyout rollback and process-teardown integration. The shared endpoint pool
   allocator remains unserialized; endpoint reclamation is absent. A NULL cap
   sink currently entails native-handler install/revoke, so it does not prove
   successful discard under receiver CSpace exhaustion.
4. Establish full object/CSpace pinning and installed-descendant invalidation on
   teardown; the locator lock secures pending transfers only. Generic cap paths
   and source CSpace teardown after an already delivered delegation remain
   outside this limited lifetime proof.
5. Implement bounded pointer-free cross-owner delivery, remote failure/rollback,
   replay and true concurrent SMP stress. Current install explicitly rejects
   foreign-owner sources rather than mutating remote CSpaces.
6. Restore allocating RPC only with safe cleanup, correct monotonic timeout
   units and reply correlation before outputs. Demonstrate positive/negative
   authenticated guest traces, 10k complete call/timeout/failure cycles and real
   service integration on the profile matrix. No such scenario currently exists.
7. Repair the full host-test build and the two existing assertion failures.

## Exact changed-file inventory and hygiene

No generated configuration/header, third-party source, build directory, firmware,
local log or scratch file is included. Syscall manifest and lock are unchanged.

```text
core/kernel/include/capability.h
core/kernel/src/cap/cap_cspace.c
core/kernel/src/cap/cap_internal.h
core/kernel/src/ipc/endpoint_ipc.c
core/lib/ipc/README.md
core/lib/ipc/include/bharat/ipc/ipc.h
core/lib/ipc/src/ipc.c
core/lib/ipc/tests/test_ipc_wire.c
core/services/servicemgr/tests/test_support.c
core/services/vm_manager/vm_manager.c
core/services/vm_manager/vm_manager.h
docs/adr/ADR-039-p0-vm-supervisor-fail-closed.md
docs/architecture/p0-ipc-envelope-proposal.md
docs/architecture/p0-ipc-identity-verification.md
docs/architecture/p0-vm-sm-verification.md
interface/include/bharat/uapi/ipc/contract.h
quality/tests/host/CMakeLists.txt
quality/tests/host/test_capability_ownership.c
quality/tests/host/test_ipc_transfer_lifetime.c
quality/tests/host/test_vm_ipc_boundary.c
```
