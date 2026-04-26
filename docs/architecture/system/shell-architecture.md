---
title: Shell Architecture and Production Contract
status: Draft
owner: Documentation Working Group
last_updated: 2026-04-25
tags:
  - docs
  - architecture
  - system
see_also:
  - README.md
---
# Shell Architecture and Production Contract

## Purpose

This document defines the canonical Bharat-OS system shell architecture and aligns it with the current console subsystem reality.

- Kernel provides primitives and enforcement.
- `core/services/system/console/` is the stream broker boundary (currently scaffold).
- `core/services/system/shell/` provides parser/dispatch/policy enforcement.
- Profile policy determines availability and command surface.

> Non-negotiable: kernel does not own shell policy.

## Runtime placement and boundaries

- Runtime shell service: `core/services/system/shell/`
- Runtime console service: `core/services/system/console/`
- Optional shared CLI helpers: `lib/cli/` or `lib/msg/cli/`
- Deferred POSIX compatibility shell: `core/personalities/compat/`

Shell handlers must use service contracts/backend APIs and must not access kernel internals directly.

## Shell classes

1. Mini shell: low-footprint recovery/factory/bring-up path.
2. Admin shell: operational diagnostics and controlled mutating commands.
3. Compatibility shell (deferred): personality-layer POSIX-style shell semantics.

## Current implementation status

### Implemented now

- Bounded parser and tokens.
- Command registry with namespaced multi-token command support.
- Capability + mode gating (`dev/prod/factory/recovery`).
- Deny path + auth lockout behavior.
- Text and `kv` output modes.
- Host tests covering parser, registry, denials, malformed input, timeout/backend failure, integration session.

### Missing / partial

- Shell main runtime loop + interactive IPC session plumbing is not complete.
- Backend now uses a uRPC request/response transport shim (`shell_backend_urpc.c`) so command handlers already execute through an IPC-shaped boundary.
- No full shell↔console daemon wiring for true end-to-end TTY/session path.
- Remote/script/compat modes are build-flag placeholders today.

## Implementation work items (tracked from this architecture)

### Completed in current shell code

- Added an interactive shell runtime loop in `shell_service.c` with bounded line input (`SHELL_MAX_INPUT_LEN`) and explicit stdout flush behavior.
- Hardened command matching to avoid fixed-size command reconstruction buffers during multi-token lookup.
- Added elapsed-time timeout enforcement using command metadata (`timeout_ms`) and timeout audit events.

### Remaining for full production contract

- Replace uRPC in-process shim with console-daemon transport wiring while preserving the same backend API contract.
- Introduce explicit session bootstrap/auth handshake prior to enabling privileged command catalogs.
- Wire cancellation-aware timeout path into backend/service calls instead of post-handler elapsed checks.
- Add QEMU run-stage evidence collection once cross-tree build blockers are resolved (currently failing at missing `core/lib/syscall/syscall_stubs.c` during target configure).

## Security and capability model

Mandatory controls:

- Per-command capability metadata.
- Deny-by-default access check.
- Audit events on denied/failed sensitive operations.
- Lockout/rate-limit behavior on repeated failures.

Session modes:

- `dev`
- `prod`
- `factory`
- `recovery`

## Production-grade contract

A shell is production-grade only if all are true:

1. Deterministic bounded parse and dispatch behavior.
2. Bounded runtime execution with timeout and cancellation behavior.
3. Strict service layering (no direct kernel shortcuts).
4. Stable status/message/payload contract with machine-readable output.
5. Tests for malformed input, unknown commands, deny/auth paths, and backend failure/degraded mode.
6. E2E validation with headless QEMU proving console-connected command path.

## Console alignment contract

The shell and console are considered aligned only when:

1. Shell backend is service-IPC based (not local stub).
2. Console service provides stream attach/write/flush over real capability-backed transport.
3. At least one read-only command and one privileged command are validated through end-to-end path (with deny-path evidence where expected).

## Build-time feature gates

- `CONFIG_SHELL_MINI`
- `CONFIG_SHELL_ADMIN`
- `CONFIG_SHELL_REMOTE`
- `CONFIG_SHELL_RECOVERY`
- `CONFIG_SHELL_SCRIPT`
- `CONFIG_SHELL_COMPAT_POSIX` (deferred)

## Production build profiles (shell-focused)

The shell service is compiled under `core/services/system/shell/` and currently defaults to:

- `CONFIG_SHELL_ADMIN=ON`
- `CONFIG_SHELL_RECOVERY=ON`
- `CONFIG_SHELL_MINI=OFF`
- `CONFIG_SHELL_REMOTE=OFF`
- `CONFIG_SHELL_SCRIPT=OFF`
- `CONFIG_SHELL_COMPAT_POSIX=OFF`

For production-grade shell rollout, define explicit build variants rather than relying on ad-hoc local toggles.

### Recommended CMake variants

```bash
# 1) Recovery-first mini shell (small footprint)
cmake --preset=x86_64-dev \
  -DCONFIG_SHELL_MINI=ON \
  -DCONFIG_SHELL_ADMIN=OFF \
  -DCONFIG_SHELL_RECOVERY=ON \
  -DCONFIG_SHELL_REMOTE=OFF \
  -DCONFIG_SHELL_SCRIPT=OFF

# 2) Operator/admin shell (default operational profile)
cmake --preset=x86_64-dev \
  -DCONFIG_SHELL_MINI=OFF \
  -DCONFIG_SHELL_ADMIN=ON \
  -DCONFIG_SHELL_RECOVERY=ON \
  -DCONFIG_SHELL_REMOTE=OFF \
  -DCONFIG_SHELL_SCRIPT=OFF

# 3) Extended diagnostics shell (lab only, not fleet default)
cmake --preset=x86_64-dev \
  -DCONFIG_SHELL_MINI=OFF \
  -DCONFIG_SHELL_ADMIN=ON \
  -DCONFIG_SHELL_RECOVERY=ON \
  -DCONFIG_SHELL_REMOTE=ON \
  -DCONFIG_SHELL_SCRIPT=ON
```

`CONFIG_SHELL_COMPAT_POSIX` remains deferred and must not be used as a blocker for core system shell production readiness.

## QEMU validation builds and run flow

To validate shell + console behavior on emulator targets, use the delivery target specs and the unified build runner:

```bash
# x86_64 baseline
python3 tools/build.py all --target-yaml delivery/targets/qemu/x86_64_desktop_headless.yaml

# arm64 baseline
python3 tools/build.py all --target-yaml delivery/targets/qemu/arm64_desktop_headless.yaml

# riscv64 baseline
python3 tools/build.py all --target-yaml delivery/targets/qemu/riscv64_desktop_headless.yaml
```

If `all` is too slow for iteration, split the stages:

```bash
python3 tools/build.py build --target-yaml delivery/targets/qemu/x86_64_desktop_headless.yaml
python3 tools/build.py package --target-yaml delivery/targets/qemu/x86_64_desktop_headless.yaml
python3 tools/build.py run --target-yaml delivery/targets/qemu/x86_64_desktop_headless.yaml
```

## Interactive shell acceptance criteria (QEMU)

A QEMU run is considered shell-ready only when all checks pass:

1. Boot reaches stable serial console output in `-nographic` mode.
2. Shell session can receive typed input and return output over console stream.
3. Read-only commands succeed (`help`, `version`, `status`, `uptime`).
4. One privileged command path is validated in both states:
   - denied path in restricted mode (expected),
   - allowed path with required capability in permitted mode.
5. Structured output (`mode kv`) emits machine-parsable response contract.
6. Negative-path checks: malformed command, unknown command, backend unavailable.

Until full shell↔console IPC is landed, keep these as mandatory milestone gates with explicit “expected gap” notes in release evidence.

## Near-term roadmap

### Phase 1 — Integration completion

- Implement IPC-backed shell backend.
- Complete console service URPC/capability request flow.
- Add shell service runtime loop that binds to console session/stream endpoints.

### Phase 2 — Hardening

- Replace simulated timeout behavior with real timeout/cancellation paths.
- Centralize audit sink integration.
- Move mode/profile command policy to explicit policy manifests.

### Phase 3 — Expansion

- Remote shell with strict auth/rate-limit posture.
- Recovery/factory differentiated command catalogs.
- Personality-scoped compatibility shell.
