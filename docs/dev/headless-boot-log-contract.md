# Headless Boot Log Contract and Parser

Bharat-OS uses a machine-readable boot contract to verify that headless QEMU/hardware runs reach their expected state. This prevents relying on manual log inspection and provides a clear PASS/FAIL signal for CI and coding agents.

## Overview

- **Contract**: A YAML file defining expected, forbidden, and skippable markers for each target.
- **Parser**: A Python tool (`tools/testing/check_boot_log.py`) that validates a log file against the contract.
- **Fixtures**: Minimalist log files used to verify the parser itself.

## Boot Contract (`quality/contracts/boot/`)

The contract defines rules per target.

```yaml
targets:
  x86_64_desktop_headless:
    required:
      - "BOOT: kernel_main reached"
      - marker: "KT: capability"
        match: substring
      - type: regex
        pattern: "\\[KT\\].*PASS"
    allowed_skip:
      - "SKIP: sched_remote_ipi requires >=2 online cores"
    forbidden:
      - "PANIC"
      - "ASSERT"
    timeout_seconds: 30
```

### Marker Matching Types

1. **Substring** (Default): Matches if the marker is found anywhere in the line.
2. **Exact**: Matches if the line (stripped of whitespace) exactly equals the marker.
3. **Regex**: Matches the line against a regular expression.

## Parser Tool (`tools/testing/check_boot_log.py`)

### Usage

```bash
python3 tools/testing/check_boot_log.py \
    --log <path_to_log> \
    --contract quality/contracts/boot/headless_boot_contract.yaml \
    --target x86_64_desktop_headless
```

### Arguments

- `--log`: Path to the boot log file to be parsed.
- `--contract`: Path to the YAML contract file.
- `--target`: The target name to look up in the contract.
- `--strict`: Enable strict mode.
- `--summary`: (Optional) Printed by default in current implementation.

### Strict Mode

When `--strict` is enabled, the parser:
1. Fails if any "suspicious" error-looking line is found (e.g., `PANIC`, `FAULT`, `segmentation fault`) even if not explicitly in the `forbidden` list.
   - Lines explicitly listed in `allowed_skip` are exempted from this check.
2. Fails if there are duplicate marker definitions in the contract.
3. Fails if the `required` list is empty (unless `allow_empty_required: true` is set in the contract).
4. Fails if the target is unknown (always true regardless of strict).

### Exit Codes

- `0`: PASS - All required markers found, no forbidden/suspicious markers found.
- `1`: FAIL - Validation failed (missing required, found forbidden, etc.).
- `2`: ERROR - Tool error (invalid contract, missing file, unknown target).

## Integration for Coding Agents and CI

Coding agents should run this tool after a QEMU boot to verify success:

```bash
# Example integration hint
./run_qemu_e2e.sh > boot.log 2>&1
python3 tools/testing/check_boot_log.py --target x86_64_desktop_headless --log boot.log --contract quality/contracts/boot/headless_boot_contract.yaml
```

If the parser returns `0`, the boot is considered successful according to the contract.
