# System Shell Testing Guide

## Scope

This guide covers the production-grade minimal system/admin shell under `services/system/shell/`.

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

`services/system/shell/tests/` includes:

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

## Runtime smoke test

After building `system_shell`, run it with stdin commands, e.g.:

```sh
printf "help\nstatus\nsvc status console\n" | ./system_shell
```

For structured output mode:

```sh
printf "mode kv\nstatus\n" | ./system_shell
```
