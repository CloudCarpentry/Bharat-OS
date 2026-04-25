---
title: Boot Test Matrix
status: Draft
owner: Documentation Working Group
last_updated: 2026-04-25
tags:
  - docs
  - architecture
  - boot
see_also:
  - README.md
---
# Boot Test Matrix

**Status**: Active
**Version**: 1.0
**Owner**: Bharat-OS QA

## 1. Scope
The Boot Test Matrix validates the `boot_info_t` conversion, error reporting, memory overlaps, policy evaluations, and QEMU e2e paths.

## 2. Test Execution
1. **Host-Side Tests**:
   - Validation overlaps: `test_boot_validate.c` validates `boot_validate_memory_map()` against overlapping configurations.
   - Command line limits: `test_boot_validate.c` verifies `boot_info_set_cmdline` cleanly truncates and sets `NULL` correctly.
   - Boot Mode: `test_boot_policy.c` resolves `foo=bar mode=recovery` safely to `BHARAT_BOOT_MODE_RECOVERY`.
   - Security constraints: `test_boot_policy.c` refuses verified states unless the hardware supports and tracks them.

2. **E2E QEMU Runs**:
   - `quality/tests/e2e/boot/run_boot_tests.sh` (legacy alias: `quality/tests/e2e/boot/run_boot_tests.sh`) simulates and parses the Multiboot2, OpenSBI, and FDT outputs against `kernel_panic` behavior and deterministic success lines (`BOOT: Normalized handoff contract established`).
