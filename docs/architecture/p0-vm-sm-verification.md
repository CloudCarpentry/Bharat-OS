# BH-P0-VM-SM-INTEGRATION verification

Status: **INCOMPLETE — draft integration only.** Base `a55497a5`, branch
`feature/p0-vm-capability-servicemgr`. Qualification performed 2026-10-29.

The canonical VM type, reserved versioned request, native type-first dispatch,
negative authorization/validation, supervisor health/lifecycle repairs, isolated
staging, honest producer failures and regression fixtures are implemented.
Working native VM lifecycle and authenticated service handoff are NOT implemented.
Do not merge this as completion of the P0 task.

## Architecture and security

See [ADR-039](../../docs/adr/ADR-039-p0-vm-supervisor-fail-closed.md) for the exact
operation/rights matrix, stage state machine, ownership model and activation
dependencies. VM operations return UNSUPPORTED after validation; no fabricated
mapping/query or teardown success is returned. No physical address is accepted
from userspace. No new mapping database, syscall number or Linux VM dependency
was introduced. Distributed destroy transaction exhaustion now quarantines safely.

The supervisor staging state machine is BEGIN -> ordered SERVICE records ->
COMMIT. Duplicate/missing/cyclic graphs discard staging without replacing active
services. Adopted processes remain STARTING until health establishes BOUND then
READY. Health identity includes independent authenticated PID, registered PID,
boot session, incarnation and increasing sequence; supervisor time is authoritative.
Wire RPC has no authenticated identity provider, so it rejects all requests after
structural validation. Init returns ENOSYS, retaining lifecycle ownership.

The generic runtime requires trusted startup identity and propagates send errors;
it never emits generic READY. Service Manager publishes the created SEND endpoint
and checks registration failure. Restart waits for PM EXITED and never spawns a
second incarnation over a still-live process. Critical failure resets stability.

## Executed verification

All commands below were executed in the isolated worktree. Clang 19, LLD 19,
LLVM objcopy, QEMU 10.0.13, SeaBIOS, iPXE and OpenSBI were installed by extracting
Debian packages under `/workspace/work/toolroot`; no system or repository toolchain
files were modified. `/workspace/work/env.sh` supplies PATH and the local library
path. Earlier attempts with missing compiler/firmware failed and were superseded
by the final qualification. GCC also passed the five focused tests before Clang
became available.

| Command/gate | Result |
| --- | --- |
| `cmake --preset host-test` (Clang) | PASS |
| Focused build: `test_vm_capability test_servicemgr host_test_servicemgr_lifecycle host_test_servicemgr_harden test_init_readiness` | PASS |
| `ctest --preset host-test -R 'vm_capability\|servicemgr\|init_readiness' --output-on-failure` | PASS, exactly 5 discovered/executed |
| IPC wire and capability format regressions, plus focused tests | PASS, 7/7 |
| `cmake --build --preset host-test --target bharat_tests/host/all -j2` | FAIL; baseline TLB transport test calls undeclared `bharat_msg_header_encode` |
| `ninja -C build/host-test -k 0 bharat_tests/host/all -j2` | FAIL; 222 compile/link commands failed, including pre-existing include-path and fixture defects |
| Complete `ctest --test-dir build/host-test --output-on-failure -j2 --timeout 20` | FAIL; 117 discovered, 39 passed, 2 failed, 76 not run (missing executables) |
| `host_test_capability_ownership` | FAIL at distributed-delegation assertion, after canonical VM authorization fixture repair; assertions preserved |
| `host_test_sched_diag` | FAIL assertion; unrelated scheduler regression |
| `python3 tools/abi/syscall_abi.py --check` | PASS; syscall ABI unchanged |
| `python3 tools/lint/check_cmake_dependencies.py` | PASS, 0 violations |
| `python3 tools/lint/check_layer_references.py` | FAIL, 11 reported violations plus 101 known baseline-debt entries; none in added/edited implementation files |
| `git diff --check` | PASS (repository-local `core.whitespace=cr-at-eol` honors existing CRLF sources) |
| `python3 tools/run_qemu_matrix.py --arch x86_64 --headless --build-only` | PASS after compiler setup and request-size portability fix |
| `python3 tools/run_qemu_matrix.py --headless --smoke --all-arch --json-report build/p0-vm-sm-matrix.json` | PASS, 5/5 CORE boot contracts |

The full suite was attempted without weakening assertions, disabling tests,
changing cross-compilation detection or extending timeouts to obtain a PASS.
The explicit 20-second limit bounded unrelated full-suite tests; all focused
checks used their normal preset. The unrelated host-suite failures remain open.

## Five-architecture evidence

| Architecture | Build | Kernel link | QEMU CORE smoke | P0 runtime integration |
| --- | --- | --- | --- | --- |
| x86_64 | BUILD_PASS | LINK_PASS | BOOT_PASS | RUNTIME_INTEGRATION_FAIL |
| ARM64 | BUILD_PASS | LINK_PASS | BOOT_PASS | RUNTIME_INTEGRATION_FAIL |
| RISC-V64 | BUILD_PASS | LINK_PASS | BOOT_PASS | RUNTIME_INTEGRATION_FAIL |
| ARM32 MMU-Lite | BUILD_PASS | LINK_PASS | BOOT_PASS | RUNTIME_INTEGRATION_FAIL |
| RISC-V32 MMU-Lite | BUILD_PASS | LINK_PASS | BOOT_PASS | RUNTIME_INTEGRATION_FAIL |

QEMU evidence shows real init, namesvc and process_manager execution and their
existing CORE startup/readiness contract. Those target YAMLs select
`BHARAT_INIT_CORE_BOOTSTRAP_ONLY`; they do not select a functioning secure
supervisor handoff or exercise native VM lifecycle. The existing CORE STABLE
marker is not supervisor runtime integration evidence. No hardware MPU execution
or real-time VM lifecycle is qualified.

## Outstanding acceptance blockers

1. VM object pinning across capability revoke/destroy; verified memory extent,
   access and ownership; canonical mapping integration and safe remote teardown.
   All six VM operations are disabled, including CREATE and actual QUERY.
   Positive lifecycle, rollback and real native CSpace guest scenarios are absent.
2. Authenticated IPC sender identity or independently verified transferred
   authority, including reply provenance. A header integer is never accepted.
3. Trusted startup process/incarnation data and a versioned launch identity path.
   Init's real process IDs are retained locally; no successful adoption is claimed.
4. Staged handoff launch-state/timing verification and genuine producer readiness.
   Internal graph/health tests use explicit fixtures and do not satisfy authentication.
5. Secure guest handoff, forged-handoff recovery and critical restart integration.
6. Complete host-suite compilation and its two observed assertion failures.

## Diff hygiene and deliverables

The original checkout and `developer` branch were not edited. Work is committed
in focused milestones and pushed to the feature branch. No generated headers,
third-party source, tool installations, build logs or firmware are committed.
The bootstrap launch diff is limited to includes and type-first routing; existing
`arch_user_entry_t entry` declarations/initializations are untouched. The VM
source appears once in each mutually exclusive `bharat_mm_vmm` selection.

Detailed file/commit inventories and raw verification logs accompany the chat
artifact bundle. The PR must stay draft and must not claim the P0 acceptance
criteria have been met.
