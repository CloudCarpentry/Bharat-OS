---
title: Shell Architecture and Production Contract
status: Proposed
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
- Backend is stubbed (`shell_backend_stub.c`) instead of live service IPC.
- No full shell↔console daemon wiring for true end-to-end TTY/session path.
- Remote/script/compat modes are build-flag placeholders today.

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
