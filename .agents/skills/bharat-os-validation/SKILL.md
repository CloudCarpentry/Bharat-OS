---
name: bharat-os-validation
description: Apply before completing Bharat-OS code, configuration, build, architecture, ABI, or documentation tasks that require executable evidence.
---

# Bharat-OS Validation Skill

## Evidence discipline

A command is PASS only when it was executed and returned the expected evidence. Missing dependencies, target definitions, YAML files, emulators, flags, or test harnesses are BLOCKED. Skipped tests are never PASS.

## Validation sequence

1. Inspect the diff and list affected subsystems/architectures/backends.
2. Run focused unit/host/stress tests for the changed behavior.
3. Run applicable linters:

```bash
python3 tools/lint/check_layer_references.py
python3 tools/lint/check_cmake_dependencies.py
```

4. For syscall/ABI work:

```bash
python3 tools/abi/syscall_abi.py --check
```

5. Run required builds:

```bash
./build.sh all --target x86_64_desktop_headless
./build.sh all --target arm64_desktop_headless
./build.sh all --target riscv64_desktop_headless
./build.sh all --target arm32_embedded_headless
./build.sh all --target powerpc_server_headless
```

6. Inspect matrix runner support:

```bash
python3 tools/run_qemu_matrix.py --help
```

7. Run the required gate when supported:

```bash
python3 tools/run_qemu_matrix.py --headless --smoke --all-arch
```

8. If `--all-arch` is not supported, run partial evidence:

```bash
python3 tools/run_qemu_matrix.py --headless --smoke
```

Mark the required all-architecture gate BLOCKED. Also verify that no required target was silently skipped.

## Completion table

Report one row per command/target:

| Gate | Command | Status | Evidence / blocker |
|---|---|---|---|

Finish with changed files, invariants, security impact, documentation updates, risks, and diff-hygiene confirmation.
