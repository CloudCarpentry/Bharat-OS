---
title: Console and Shell Architecture (Aligned Runtime Contract)
status: Proposed
owner: Documentation Working Group
last_updated: 2026-04-25
tags:
  - docs
  - architecture
  - console
see_also:
  - README.md
---
# Console and Shell Architecture (Aligned Runtime Contract)

This document defines the **current Bharat-OS console + shell architecture contract** and aligns it with the code that exists today.

It supersedes older assumptions that a userspace console already provides full TTY/session multiplexing.

## 1) Executive summary

- Kernel console is the only production-meaningful output path today (`core/kernel/src/console/*`).
- `core/services/system/console` exists, but is still a scaffold with TODO URPC routing and mock capability-acquisition flow.
- `core/services/system/shell` exists and is substantially ahead of console service in maturity (bounded parser, capability-aware dispatch, tests), but it currently runs against a stub backend and is not yet wired to console URPC.
- Therefore, **console and shell are architecturally compatible but not yet fully integrated at runtime**.

## 2) Layering contract (must remain stable)

## 2.1 Kernel mechanism (trusted core)

The kernel owns mechanism only:

- early/runtime/panic console phases,
- bounded formatting/log routing,
- backend selection + fallback,
- panic-safe flush behavior.

Code anchors:

- `core/kernel/src/console/console_core.c`
- `core/kernel/src/console/console_policy.c`
- `core/kernel/src/console/console_buffer.c`
- `core/kernel/src/console/serial_console.c`
- `core/kernel/src/console/framebuffer_console.c`

## 2.2 Userspace console service policy/runtime brokering

`core/services/system/console/main.c` is intended to own:

- stream brokering (`stdin/stdout/stderr` style channels),
- session/TTY attachment,
- URPC routing,
- capability-mediated output dispatch.

Current state: scaffold loop + placeholder IPC calls.

## 2.3 Userspace shell service command plane

`core/services/system/shell/` owns:

- line parsing,
- command registry/dispatch,
- per-session mode/capability checks,
- response formatting (text + `kv` machine-readable mode).

Current state: implemented and unit-tested on host; still backend-stub based.

## 3) Current alignment between console and shell

## 3.1 What is aligned now

- Layering intent is correct: shell does not poke kernel internals directly.
- Shell command handlers are designed around service/backend abstraction (`shell_backend_api_t`).
- Console service and shell both target capability-mediated service composition.

## 3.2 What is not aligned yet

1. No concrete shell↔console transport binding (no URPC session plumbing from shell into console daemon).
2. Console daemon does not yet implement real `bharat_ipc_call`/`bharat_ipc_send` flow for write/flush and stream ops.
3. No shared contract document for shell session-to-console stream model (PTY/TTY/session lifecycle).
4. End-to-end boot validation does not yet prove interactive shell over console on QEMU; only baseline boot/log behavior is generally testable.

## 4) Shell implementation status (as of this update)

Implemented in `core/services/system/shell/`:

- bounded parser (`SHELL_MAX_INPUT_LEN`, `SHELL_MAX_TOKENS`),
- capability/mode gated dispatch,
- lockout after repeated denied auth path,
- structured output mode (`mode kv`),
- command registry for baseline read-only + privileged commands,
- host-side tests for parser/registry/denial/malformed/timeout/integration behavior.

Not yet implemented (or intentionally stubbed):

- real IPC-backed backend (currently `shell_backend_stub.c`),
- interactive service loop main path (today `main` in `shell_service.c` is placeholder),
- remote shell transport,
- script mode,
- compatibility POSIX shell (deferred to personality layer).

## 5) Missing code to implement next (priority order)

### P0 — Required for real console+shell integration

1. **Console daemon IPC implementation**
   - Replace mock return path in `console_acquire_output_capability`.
   - Implement real send/call path for write/flush opcodes.
2. **Shell backend over service IPC**
   - Add `shell_backend_ipc_console.c` (or equivalent) implementing backend API via console + system services.
3. **Session wiring**
   - Introduce explicit shell session lifecycle + binding to console stream IDs.

### P1 — Reliability and policy hardening

4. Add command timeout enforcement with cancellation hooks (not busy-loop simulation).
5. Add comprehensive audit sink integration for denied/failed privileged operations.
6. Add production-mode policy tables externalized from code (profile policy files).

### P2 — Feature completion

7. Remote shell transport gate (`CONFIG_SHELL_REMOTE`) with auth and rate limits.
8. Recovery/factory distinct command profiles and test matrix.
9. Compatibility shell landing zone in `core/personalities/compat/` (kept separate from system shell).

## 6) Unified roadmap (console + shell)

## Phase A (near-term): bootstrap integration

- Complete console daemon URPC plumbing.
- Implement shell IPC backend.
- Prove shell command round-trip to real backend on QEMU headless.

## Phase B (mid-term): production policy and resiliency

- Audit/event pipelines for privileged paths.
- Deterministic timeout/cancellation in dispatch.
- Profile-specific command policy manifests.

## Phase C (later): scale and compatibility

- Session multiplexing + PTY-like abstractions where needed.
- Remote shell endpoint with strict policy.
- Personality-scoped POSIX compatibility shell.

## 7) Headless QEMU validation contract

Minimum acceptance evidence for console/shell alignment work:

1. QEMU boots in `-nographic` mode and emits kernel console logs.
2. Console service starts and reports backend acquisition (success/failure with explicit code).
3. Shell service starts with backend availability state.
4. At least one command path (`status`/`svc list`) proves end-to-end response through service backend (not local stub).

Recommended command style:

```bash
timeout 30s python3 tools/build.py all --target-yaml tools/targets/qemu/x86_64_desktop_headless.yaml
```

For now, when (4) is not yet implemented, mark this as expected gap and keep shell host-tests mandatory.

## 8) Invariants (non-negotiable)

- Kernel keeps mechanism; shell/console services keep policy and session behavior.
- Panic path remains lockless/allocation-free/polling-safe.
- Shell parser/dispatch remains bounded.
- Compatibility/POSIX semantics never leak into core system shell path.
