---
title: End-to-End Testing
status: Draft
owner: Documentation Working Group
last_updated: 2026-04-25
tags:
  - docs
  - testing
see_also:
  - README.md
---
# End-to-End Testing

Bharat-OS uses a two-layer E2E testing approach focusing on QEMU CI matrices and manual hardware testing paths. Due to the profile-driven architecture of the OS, the test matrix validates combinations of architecture and device/personality profiles.

## QEMU Matrix

The current QEMU E2E test matrix is defined in `quality/tests/e2e/qemu_matrix.json` (legacy alias: `quality/tests/e2e/qemu_matrix.json`) and split into two stages:
1. **Smoke Tests:** Stable, representative architectures and GP profiles (e.g., `x86_64-gp`, `arm64-gp`, `riscv64-gp`).
2. **Extended Tests:** Slower or secondary combinations (e2e-qemu-extended).

These run on GitHub Actions automatically on push and PR.

## Manual Hardware Testing

Physical board testing is triggered via `workflow_dispatch` through self-hosted runners. Hardware testing currently remains manual to avoid the flakiness of trying to validate real boards on standard GitHub-hosted VMs.

## Success Criteria

Pass/fail is determined by analyzing the serial log output from the kernel boot process using `quality/tests/e2e/assert_log.py` (legacy alias: `quality/tests/e2e/assert_log.py`).
It specifically looks for canonical markers in stdout, like `BOOT: pmm initialized`, while ensuring there are no `[PANIC]` messages.

---

## Running Locally

There are two ways to run the E2E tests locally: natively via scripts, and using `act-cli` to simulate GitHub Actions.

### 1. Running the script directly

You can invoke the Python script for any single profile defined in `quality/tests/e2e/profiles/` (legacy alias: `quality/tests/e2e/profiles/`):

```bash
./quality/tests/e2e/run_e2e.py quality/tests/e2e/profiles/x86_64-gp.yaml
# legacy alias still works during migration:
./quality/tests/e2e/run_e2e.py quality/tests/e2e/profiles/x86_64-gp.yaml
```

The script will configure CMake, build the kernel, boot QEMU with a 15-second timeout, and assert the serial logs against the required markers. The logs are saved in the `e2e_logs/` directory.

### 2. Simulating GitHub Actions with act-cli

`act` is excellent for fast local workflow validation, especially checking YAML syntax, job dependencies, and simple Linux/QEMU actions.

**Note:** Nested virtualization or privileged runner behaviors might differ slightly from real GitHub-hosted VMs, so `act` does not replace a final CI run.

#### Requirements for `act`
1. Docker installed and running.
2. `act-cli` installed (e.g., `brew install act` on macOS, or via GitHub releases).

#### How to run a specific job

To run only the `e2e-qemu-smoke` job locally:

```bash
act -j e2e-qemu-smoke
```

#### How to run a single matrix entry

To avoid running the entire matrix, use the `--matrix` flag:

```bash
act -j e2e-qemu-smoke --matrix arch=x86_64 --matrix profile=x86_64-gp.yaml
```

#### Artifacts / Bind Mounts

`act` provides artifact support. If you want to keep the uploaded artifacts (the `e2e_logs/` folder), you can pass an artifact server path:

```bash
act -j e2e-qemu-smoke --artifact-server-path /tmp/artifacts
```

#### Skipping hardware jobs

By default, the `e2e-hardware-manual` job only triggers on `workflow_dispatch`. When running `act` locally, you may want to explicitly run a `push` or `pull_request` event, naturally skipping the `workflow_dispatch`-only manual jobs:

```bash
act push
```
