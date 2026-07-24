---
title: Shell Contributor Guide
status: Draft
owner: Documentation Working Group
last_updated: 2026-04-25
tags:
  - docs
  - dev
see_also:
  - README.md
---
# Shell Contributor Guide

This guide defines contribution rules for Bharat-OS shell work. Follow this before opening implementation PRs.

## Scope

Applies to:

- `core/services/system/shell/` (runtime, parser, dispatch, output, auth)
- optional shared helpers in `lib/cli/` or `lib/msg/cli/`
- shell architecture/profile docs in `docs/architecture/system/`

## Ownership and boundaries

### Do

- Keep system shell runtime in `core/services/system/shell/`.
- Integrate command handlers through service contracts (IPC/service APIs).
- Add explicit capability metadata for each privileged command.
- Maintain bounded parsing and bounded memory behavior.
- Implement machine-readable output mode for automation.
- Add unit/integration tests for parser, dispatch, and auth paths.

### Don't

- Do not implement shell policy inside kernel code.
- Do not directly access kernel internals from shell handlers.
- Do not add Linux/POSIX semantics to the core system shell path.
- Do not add unbounded allocations or unbounded token/line parsing.
- Do not add privileged commands without audit and deny-path tests.

## Architecture contract (must preserve)

1. Kernel owns primitives and enforcement mechanisms, not shell policy.
2. Shell service owns command parsing, dispatch, and per-profile policy evaluation.
3. Compatibility/POSIX shell is deferred and belongs under `core/personalities/compat/`.

## Command design requirements

Each command should declare:

- Namespace (`sys`, `svc`, `log`, `dev`, `net`, `proc`, `mem`, `cap`, `health`, `update`).
- Required capabilities (or explicit read-only/none).
- Supported modes (`dev`, `prod`, `factory`, `recovery`).
- Timeout behavior and dependency requirements.
- Output contract (text + structured form).

## Security requirements

- Capability-gated dispatch for privileged commands.
- Deny-by-default for missing capability/mode permissions.
- Audit events for denied, failed-auth, and sensitive command execution.
- Lockout and rate-limit behavior for repeated auth failures.

## Production-grade checklist

PRs touching shell runtime should verify:

- [ ] Deterministic parser with bounded input and tokens.
- [ ] Non-blocking runtime loop or equivalent cooperative model.
- [ ] Command timeout/cancellation path.
- [ ] Stable error code + message contract.
- [ ] Structured output mode available.
- [ ] Safe degraded behavior when backend service is unavailable.
- [ ] No direct kernel-internal shortcuts.
- [ ] Tests for malformed input, unknown commands, deny paths, and failure paths.

## Suggested implementation layout

- `core/services/system/shell/include/`
- `core/services/system/shell/src/`
- `core/services/system/shell/quality/tests/`
- `core/services/system/shell/src/commands/`

## Review guidance

A shell PR should be rejected if it:

- couples handlers to kernel internals,
- lacks capability metadata for privileged paths,
- introduces unbounded parser/runtime behavior,
- omits tests for denial and malformed input,
- or expands scope into POSIX compatibility without personality-layer design approval.

## Related docs

- `docs/architecture/system/shell-architecture.md`
- `docs/architecture/system/shell-profile-matrix.md`
