# Shell Architecture and Production Contract

## Purpose

This document defines the canonical Bharat-OS shell architecture and ownership model. It standardizes shell behavior across device profiles while preserving strict layering:

- Kernel provides primitives and capability enforcement.
- `services/system/shell/` provides shell mechanism and command dispatch.
- Product/profile policy decides which shell class is enabled.

> **Non-negotiable boundary:** the kernel must not own shell policy. Shell policy is a service-layer concern.

## Shell taxonomy

Bharat-OS defines three shell classes:

1. **Mini shell**
   - Target: UART bring-up, recovery, factory, emergency service mode.
   - Scope: tightly bounded command set; mostly diagnostics and static status.
   - Footprint: smallest memory/runtime budget.

2. **Admin shell**
   - Target: field operations, system administration, diagnostics.
   - Scope: service lifecycle, health, log inspection, network/device/process status.
   - Footprint: moderate; capability-gated mutating operations.

3. **Compatibility shell** (deferred)
   - Target: Linux/POSIX personality workloads.
   - Scope: personality-facing interactive shell semantics.
   - Placement: `personalities/compat/` (not in core system shell service).

## Layering and placement

### Required placement

- **Runtime service:** `services/system/shell/`
- **Shared parser/output helpers (optional):** `lib/cli/` or `lib/msg/cli/`
- **Compatibility/POSIX shell (later):** `personalities/compat/`

### Layering rules

- Command handlers **must** call stable service contracts (IPC/IDL or service APIs).
- Command handlers **must not** access kernel internals directly.
- Shell command policy (allow/deny by profile/mode) belongs to shell service/profile config, not kernel code.

## Security and capability model

Shell dispatch is capability-mediated.

### Core requirements

- Each command declares required capability rights.
- Session carries authenticated role and effective capability mask.
- Dispatcher enforces deny-by-default for privileged commands.
- Denied and failed privileged attempts generate audit records.

### Session roles/modes

Minimum modes:

- `dev` (developer mode)
- `prod` (restricted production mode)
- `factory` (provisioning/manufacturing)
- `recovery` (break-glass support)

### Mandatory controls

- Failed-auth and denied-command logging.
- Lockout policy after repeated auth failure.
- Rate limiting on auth and sensitive command classes.
- Recovery/factory mode restrictions are explicit and build/profile controlled.

## Production-grade shell requirements

A shell implementation is production-grade only if it satisfies all of the following:

1. **Deterministic parser behavior**
   - Bounded line length and token count.
   - Stable parsing result for identical input.
2. **Bounded runtime model**
   - Non-blocking event loop.
   - Timeout and cancellation contract for command handlers.
   - No unbounded buffers or growth.
3. **Strict layering**
   - No direct kernel poking from command handlers.
4. **Stable output and error contract**
   - Standard status code + message.
   - Optional structured payload (machine-readable mode).
5. **Testability**
   - Parser, dispatch, auth, malformed input, and degraded-mode tests.
6. **Safe degraded behavior**
   - Commands fail safely when dependent services are unavailable.

## Build/profile integration contract

Use build-time configuration gates to include only required shell features:

- `CONFIG_SHELL_MINI`
- `CONFIG_SHELL_ADMIN`
- `CONFIG_SHELL_REMOTE`
- `CONFIG_SHELL_RECOVERY`
- `CONFIG_SHELL_SCRIPT`
- `CONFIG_SHELL_COMPAT_POSIX` (deferred personality target)

Profiles may additionally disable mutating command namespaces by policy.

## Command namespace contract

Recommended namespace families:

- `sys`
- `svc`
- `log`
- `dev`
- `net`
- `proc`
- `mem`
- `cap`
- `health`
- `update`

New commands should be namespaced; global top-level commands should be limited to essentials (for example `help`, `version`, `status`).

## Initial command policy model

Policy is profile/mode-driven:

- **Always allowed in dev:** core read and controlled diagnostics.
- **Read-only in prod:** status, inventory, and observability commands.
- **Factory-only:** provisioning, calibration, secure enrollment flows.
- **Recovery-only:** break-glass repair and rollback.
- **Disallowed on safety-critical runtime builds:** commands that alter live safety behavior unless explicitly approved by profile policy.

## Implementation scope note

This document defines architecture and contract, not POSIX shell behavior. The first production shell for Bharat-OS should be a minimal capability-aware system/admin shell, with POSIX compatibility deferred to personality-layer work.
