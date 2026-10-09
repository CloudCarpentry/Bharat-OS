---
title: BOOT-FLOW-P0-001 baseline and failure preservation
status: Blocked
owner: Kernel and Services Working Group
last_updated: 2026-10-10
tags:
  - boot
  - validation
---

# BOOT-FLOW-P0-001 recovery evidence

This is a partial recovery, not completion of the real-userspace bootstrap gate.
The baseline is `developer` commit
`79b8441ccfc2202e0041de30f5c3ba1703405372`, on working branch
`fix/boot-flow-p0-001-userspace-entry`.

## Reproduced behavior and root cause

The baseline x86_64 target builds and packages. QEMU reaches init's `_start`,
runtime initialization, and `main`; diagnostic syscalls return. The ELF entry
is `0x202680`, in an executable PT_LOAD. Runtime evidence shows
`rsp=0x3fffffeff8`, `arg0=0x3ffffed000`, and matching actual/expected CR3.
There is no observed first user-entry exception in this trace. This evidence
does not establish namesvc entry: the package includes only init.

`core/services/core/init/init_manifest.c:13` returned success for namesvc and
process_manager without launching either. `init_runtime.c:213` tries CORE once;
no readiness receiver populates `observed_ready`. Missing readiness sets safe
mode and rolls back namesvc, explaining its DISABLED status. At `finish`, init
nevertheless attempted supervisor handoff. A missing supervisor changed safe
mode to degraded/quiescent. Init then incorrectly printed graph completion:

```text
USER_INIT: ENTERED
USER_INIT: STARTUP_ABI_OK
BOOTAUTH:SELF_PROCESS_CAP_OK
BOOTAUTH:BOOTSTRAP_CAP_OK
BOOTAUTH:NAMESVC_CAP_OK
namesvc: DISABLED
process_manager: WAITING_DEPS
services/init: HANDOFF_DEFERRED (supervisor unavailable).
USER_INIT: SERVICE_GRAPH_COMPLETE
BOOT_RUNTIME: DEGRADED
```

## Changes

Required-service safe mode now returns an error before handoff. Init emits
`BOOT_FAIL: INIT_BOOTSTRAP` on failure and cannot emit graph completion on that
path. Bootstrap launch placeholders return `-ENOSYS` rather than falsely claim
success. The x86_64 smoke contract requires all six service evidence markers
specified in the task. Regression fixtures are synthetic parser inputs only.

The final x86_64 run reports `namesvc: FAILED`, keeps process_manager in
`WAITING_DEPS`, emits `BOOT_FAIL: INIT_BOOTSTRAP`, and exits the smoke runner
with failure on that forbidden marker. It emits neither graph completion nor
stable boot. This proves failure visibility, not recovery of service execution.

The changes keep lifecycle policy in init and preserve the capability and
syscall ABI boundaries. No kernel READY state, fake endpoint, or capability
copy was added. No generated, toolchain, third-party, or runtime-log artifacts
are included in the change.

## Validation

Activate workspace tools with `source /workspace/tooling/activate.sh` and work
in `/workspace/Bharat-OS`. The tools were installed from signed Debian package
metadata with normal package checksum validation; no root installation or
verification bypass was used.

PASS: the new six-case failure-preservation regression. Its baseline fails the
assertion that handoff is never called after required-service failure.

```bash
gcc -std=c11 -Iinterface/include -Icore/lib/runtime/include \
  quality/tests/init/test_init_failure_handoff.c \
  core/services/core/init/{init_runtime,init_graph,init_events,init_rollback,init_status,init_profile}.c \
  -o /workspace/tooling/test_init_failure_handoff
/workspace/tooling/test_init_failure_handoff
```

PASS: existing init tests plus two bootstrap-placeholder checks. The original
manifest fails the new assertion rejecting a successful no-op launch.

```bash
gcc -std=c11 -Iinterface -Iinterface/include \
  -Icore/lib/runtime/include -Icore/lib/cap/include \
  -Icore/lib/ipc/include -Icore/lib/namesvc/include \
  quality/tests/host/test_init.c \
  core/services/core/init/{init_manifest,init_runtime,init_graph,init_events,init_rollback,init_status,init_profile}.c \
  -o /workspace/tooling/test_init
/workspace/tooling/test_init
```

PASS: `bash tools/testing/test_check_boot_log.sh` (10 parser tests).

PASS: `python3 tools/lint/check_layer_references.py` (existing baseline debt,
exit 0), `python3 tools/lint/check_cmake_dependencies.py` (zero violations),
`python3 tools/abi/syscall_abi.py --check`, and `git diff --check`.

BLOCKED: `cmake --preset host-test` fails on existing stale source paths,
including `core/lib/base/src/string.c` and `../../kernel/src/profile/profile.c`.
The regression is registered as `host_test_init_failure_handoff`, but no CTest
execution is claimed; the direct test commands above executed successfully.

FAIL: `python3 tools/run_qemu_matrix.py --headless --smoke --all-arch`.
All five architecture emulators are installed. The x86_64 and ARM64 lanes
timeout before satisfying their boot contracts; RISC-V64 reports a required
bootstrap failure. ARM32 and RISC-V32 fail compiling
`core/services/system/boot_displayd/tests/test_boot_displayd.c` because the
bare-metal build cannot find host `assert.h`. These are repository failures,
not missing emulators. The canonical five `tools/build.sh all --target-yaml
delivery/targets/qemu/<target>.yaml --smoke` commands were also run separately.

## Remaining recovery prerequisites

The repository has no bootstrap launch syscall, capability invocation backend,
or service readiness transport corresponding to the referenced Jules work.
The native syscall manifest has no bootstrap launch operation; the weak
`cap_invoke` in `core/kernel/src/trap/trap.c:90` returns unsupported.
`tools/package/packager.py:142` packages one root, not the required service
executables. `core/services/process_manager/main.c:11` exits without installed
kernel operations; it also retains a fake endpoint at line 15. Namesvc has no
explicit READY reporting. These components must be recovered from existing
work or implemented under a separately resolved mechanism/ABI scope before
the real-process acceptance gate can pass. This patch does not implement them.

The environment draft stores tested installation and activation/startup
instructions. Saving that draft does not publish the snapshot or establish
fresh-task restoration. No PR is submitted while the mandatory gate is failing.
