# System Shell Testing Guide

## Scope

This guide covers the production-grade minimal system/admin shell under `core/services/system/shell/` and its alignment path with `core/services/system/console/`.

## Build enablement

Enable shell service build in CMake:

- `-DBHARAT_ENABLE_SYSTEM_SHELL=ON`
- Optional feature gates:
  - `-DCONFIG_SHELL_MINI=ON`
  - `-DCONFIG_SHELL_ADMIN=ON`
  - `-DCONFIG_SHELL_REMOTE=OFF` (default)
  - `-DCONFIG_SHELL_RECOVERY=ON`
  - `-DCONFIG_SHELL_SCRIPT=OFF` (default)

## Commands covered

Read-only baseline commands:

- `help`
- `version`
- `uptime`
- `echo`
- `status`
- `sys info`
- `svc list`
- `svc status <name>`
- `log tail`
- `health summary`
- `dev list`
- `mem stat`

Privileged examples:

- `reboot` (capability-gated)
- `diag run` (capability-gated; timeout path exercised)

## Output modes

- Human-readable text (default)
- Key-value structured mode (`mode kv`)

## Host test coverage

`core/services/system/shell/quality/tests/` includes:

- parser bounds/malformed input tests
- registry sanity tests
- permission deny-path and auth lockout tests
- backend unavailable and timeout-path tests
- integration session test for command processing and structured output

## Security checks

Verify:

- deny-by-default on missing capabilities,
- denied command audit events,
- auth lockout after repeated denied attempts,
- restricted behavior in production mode.

## Runtime smoke test (host binary)

After building `system_shell`, run it with stdin commands, e.g.:

```sh
printf "help\nstatus\nsvc status console\n" | ./system_shell
```

For structured output mode:

```sh
printf "mode kv\nstatus\n" | ./system_shell
```

## Headless QEMU validation (alignment gate)

Use at least one headless QEMU target to verify boot + console path:

```sh
timeout 30s python3 tools/build.py all --target-yaml tools/targets/qemu/x86_64_desktop_headless.yaml
```

Current expected reality:

- kernel console logs should appear,
- shell host tests should pass,
- full interactive shell-over-console E2E may still be incomplete until console daemon IPC and shell IPC backend wiring are implemented.

Treat missing interactive shell-over-console behavior as a tracked integration gap, not a parser/dispatch correctness failure.
