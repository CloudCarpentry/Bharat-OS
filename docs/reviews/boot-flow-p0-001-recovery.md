---
title: BOOT-FLOW-P0-001 real userspace bootstrap recovery
status: Implemented
owner: Kernel and Services Working Group
last_updated: 2026-10-09
tags: [boot, validation]
---

# BOOT-FLOW-P0-001 recovery evidence

Base: `developer` SHA `79b8441ccfc2202e0041de30f5c3ba1703405372`.
Working branch: `fix/boot-flow-p0-001-userspace-entry`. Inspection of available
remote history/branches found no Jules launch/readiness implementation to reuse.
The authorized recovery therefore connects existing capability invocation, ELF
loading, scheduling, endpoint IPC and monotonic time mechanisms. No native
syscall number, framework, generated ABI file or third-party source was added.

## Root cause and execution evidence

The baseline packages only init. Its manifest returns success for namesvc and
process_manager without creating either process. CORE is evaluated before any
readiness receiver can populate state. Required failure can then be overwritten
by degraded supervisor handoff, producing misleading graph-completion output.
The first baseline x86 run reached init's `_start` and `main` with diagnostic
syscalls returning; it did not execute namesvc. Initial ELF entry was
`0x202680`, startup pointer `0x3ffffed000`, and stack `0x3fffffeff8`.

After real launch was restored, GDB captured an x86 instruction-fetch page fault
(error `0x15`, CR2/PC `0x204405`). Init and namesvc reused the same low virtual
image addresses and shared the lower page-table hierarchy; loading the child
replaced init's executable mapping with an NX page. The stack allocation was
not the root cause. Private process tables and a user image base outside kernel
identity text remove both collisions.

The final source boundaries are:

- `core/services/core/init/init_manifest.c:11`: real bootstrap launch calls.
- `core/kernel/src/init/bootstrap_launch.c:42`: typed owner-local invocation,
  real ELF/thread creation and CSpace delegation; launch does not imply READY.
- `core/arch/x86/x86_64/hal_pt_x86_64.c:98`: private lower-half hierarchy,
  supervisor inheritance, allocation rollback and cleanup.
- `delivery/cmake/modules/BharatCommon.cmake`: x86 user image base `0x40000000`.
- `core/kernel/CMakeLists.txt:40`: generated configuration include precedence.
- `core/kernel/src/sched/sched_wait.c:53`: blocked IPC actually reschedules while
  preserving the outgoing thread context.
- `core/kernel/src/trap/syscall_gate.c:264`: canonical native status translation
  even when optional compatibility-personality registration is disabled.
- `core/kernel/src/personality/native/native_syscall_handlers.c`: endpoint
  payloads use bounded kernel buffers and fault-safe usercopy. Direct kernel
  access to user buffers had faulted on RISC-V with supervisor access disabled.
- `core/services/core/init/init_runtime.c`: validated BOUND/READY event loop,
  monotonic deadlines, dependency reevaluation and failure-preserving handoff.
- `core/services/namesvc/main.c`, `core/services/process_manager/main.c`:
  readiness comes from executing services with child-local receive endpoints.

ARM64 GDB captured early alignment faults in compiler-generated accesses before
RAM had Normal attributes; strict-alignment compilation fixes this boundary.
ARM64 SVC hardware already advances ELR; adding four again skipped the runtime
syscall stub's return. RISC-V64's initial page tables lacked the high supervisor
aliases assumed by its physmap backend. ARM32 first faulted on an unaligned
console halfword; after that fix GDB captured a branch to zero following syscall
scheduling. Its trap return had not restored banked user SP/LR. The ARM32 return
now restores that pair and its context-switch C call keeps AAPCS alignment.

Both real services now execute on all five architectures. A representative
QEMU trace is:

```text
NAMESVC_USER_ENTRY
NAMESVC_MAIN_ENTER
BOOTAUTH:NAMESVC_BINDING_OK
NAMESVC_READY
PROCESS_MANAGER_LAUNCH
PROCESS_MANAGER_READY
BOOT_CLASS_CORE_READY
namesvc: READY
process_manager: READY
USER_INIT: SERVICE_GRAPH_COMPLETE
BOOT_RUNTIME: STABLE
```

Init starts process_manager after consuming namesvc's actual READY event on its
dedicated receiver. All five contracts require these six service/core markers
and reject fatal errors, including `BOOT_FAIL:`. Parser fixtures are synthetic
negative inputs; the passing evidence above comes from QEMU, not fixtures.

## Validation results

Work in `/workspace/Bharat-OS` after `source /workspace/tooling/activate.sh`.
The workspace has CMake 3.31, LLVM/Clang 19, GDB multiarch, DT tooling and all five
QEMU 10 emulators, extracted from signature/checksum-validated Debian packages.
No root installation or verification bypass was needed.

| Command | Result |
| --- | --- |
| `./tools/build.sh all --target-yaml delivery/targets/qemu/x86_64_desktop_headless.yaml --smoke` | PASS |
| `./tools/build.sh all --target-yaml delivery/targets/qemu/arm64_desktop_headless.yaml --smoke` | PASS |
| `./tools/build.sh all --target-yaml delivery/targets/qemu/riscv64_desktop_headless.yaml --smoke` | PASS |
| `./tools/build.sh all --target-yaml delivery/targets/qemu/arm32_mmu_lite_headless.yaml --smoke` | PASS |
| `./tools/build.sh all --target-yaml delivery/targets/qemu/riscv32_mmu_lite_headless.yaml --smoke` | PASS |
| `python3 tools/run_qemu_matrix.py --headless --smoke --all-arch` | PASS, five lanes |
| `CC=clang bash tools/testing/test_bootstrap_recovery.sh` | PASS, nine executables, component-policy cases and 18 parser tests |
| `python3 tools/lint/check_layer_references.py` | PASS with recorded baseline debt (19 findings, baseline 109) |
| `python3 tools/lint/check_cmake_dependencies.py` | PASS, zero violations |
| `python3 tools/abi/syscall_abi.py --check` | PASS, unchanged compatibility lock |
| `cmake --preset host-test` | FAIL: pre-existing stale source paths; broad CTest suite BLOCKED |

Focused tests exercise required failure/handoff (six cases), real event/dependency
progression, timeout and malformed events, native error codes, endpoint copy
faults and transferred-cap cleanup, x86 isolation and allocation rollback,
malformed boot bundles, process-manager transaction compensation and 1,000
spawn/terminate/reap cycles with zero tracked resource leaks. Headless component
selection preserves graphical/infotainment requirements. Kernel runtime
selftests report `run=7 pass=7 fail=0` in the passing QEMU trace.

Regression evidence against pre-fix code is negative: the required-failure test
previously asserted on handoff; the native-status test asserts on unnormalized
AGAIN; the x86 isolation test asserts on shared-table/rollback behavior; the
endpoint test asserts when IPC receives the user pointer directly. Current
implementations pass. Expected baseline assertion failures are not current test
failures.

Local detailed run evidence remains under `/workspace/tooling/`; qualification
JSONs are under `build/evidence/`. Build products/logs are intentionally excluded
from Git. The broad host preset fails on unrelated paths such as
`core/lib/base/src/string.c` and `../../kernel/src/profile/profile.c`; no passing
CTest or complete repository-suite result is claimed.

## Architecture, scope and remaining limitations

The explicit `BHARAT_INIT_CORE_BOOTSTRAP_ONLY` policy selects and packages the
P0 graph of init, namesvc and process_manager. CORE readiness is truthful for
that graph; it does not qualify all production services, general service RPC,
restart/supervisor policy, graphical targets, SMP launch, MPU-only targets, or
non-QEMU boards. Existing larger service RPCs can exceed endpoint payload limits.
The native manager backend is installed and capability-probed before READY.

Capabilities are typed, generation-checked, owner-local and delegated between
CSpaces with attenuated rights. Self-process roots are revoked before process
slot reuse. Rollback attempts real termination/reap and visibly quarantines
failure. The existing endpoint backend cannot cancel blocked waiters or reclaim
all failed-launch endpoints; blocked termination returns unsupported and bounded
resources may remain quarantined. No false cleanup is reported. General endpoint
lifecycle work is a follow-up, not a claim of this P0 gate.

ADR-036, the contract index, init README and BUILD.md document this boundary.
Generated configuration/syscall headers, build outputs, toolchain files, logs,
third-party files and unrelated refactors are excluded from the delivered diff.
The environment draft saves installation and startup instructions; saving does
not publish or verify restoration in a new cloud task.
