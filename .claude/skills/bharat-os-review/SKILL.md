---
name: bharat-os-review
description: Apply when conducting pull request, code review, or pre-merge architectural hygiene audit on Bharat-OS changes.
---

# Bharat-OS Code & Architecture Review Skill

## Goal

Provide a rigorous, automated-first architectural and security review before any code is merged into Bharat-OS.

## Review Audit Checklist

### 1. Architectural Boundaries & Placement
- [ ] Are kernel mechanisms strictly isolated from userland policies?
- [ ] Is ISA-specific assembly confined to `core/arch/`?
- [ ] Are drivers placed in `core/drivers/` and not embedded in `core/kernel/` or services?
- [ ] Does `python3 tools/lint/check_layer_references.py` pass with zero violations?
- [ ] Does `python3 tools/lint/check_cmake_dependencies.py` pass?

### 2. Distributed Operations & Concurrency
- [ ] Does every mutable object have a documented single owner core?
- [ ] Are wire structures fixed-width (`uint32_t`, `uint64_t`) with `static_assert` for size and alignment?
- [ ] Are any raw pointers, function pointers, or `long`/`size_t` fields crossing IPC/wire boundaries? (Reject if yes!)
- [ ] Do deadlines use monotonic HAL time (`bh_ktime_now()`)?
- [ ] **Are there any unbounded spin loops or spin-counter loops?** (e.g. `for (int i=0; i<timeout_loops; i++) arch_cpu_relax();` is strictly illegal).
- [ ] Are locks released before waiting for remote ACKs/replies?
- [ ] Is destruction mediated by a terminal lifecycle state (e.g. `DYING`) that rejects new operations?

### 3. Memory & Backend Neutrality
- [ ] Does the change support `MMU_FULL`, `MMU_LITE`, and `MPU/PMP` profiles without assuming page tables exist?
- [ ] Does it fail closed (`BH_STATUS_ERR_UNSUPPORTED`) on unsupported combinations?
- [ ] Are user pointers validated and copied via fault-safe usercopy before dereferencing?
- [ ] Does `python3 tools/lint/check_no_recursive_memops.py` pass?

### 4. Capabilities & Syscall ABI
- [ ] Is every new operation capability-mediated?
- [ ] Are embedded capability handles inside user structs validated ONLY after usercopy into kernel memory?
- [ ] If syscalls were added/modified, does `python3 tools/abi/syscall_abi.py --check` pass?
- [ ] Are there any hardcoded syscall numbers or bypass tables? (Reject if yes!)

### 5. Diff Hygiene
- [ ] Are generated files (e.g. `bharat_config.h`, build directories, temporary logs) excluded from the diff?
- [ ] Are third-party directories (`ext/`, `external/`) untouched unless explicitly requested?
- [ ] Are all TODOs assigned a concrete issue or ticket rather than vague "fix later"?

### 6. Executable Verification Evidence
- [ ] Were focused tests executed and their command outputs verified?
- [ ] Was the full 5-target matrix executed via `python3 tools/run_qemu_matrix.py --headless --smoke --all-arch`?
- [ ] Are all outcomes reported honestly as `PASS`, `FAIL`, or `BLOCKED`?
