# Boot qualification triage refresh — 2026-08-05

This note refreshes the older deep-audit ordering against the current worktree. It is a planning artifact, not qualification evidence.

## Current confirmation

The current branch still contains the Gate 1 boot-truth hazards:

- `tools/package/packager.py` still creates a `MOCK_PAYLOAD_<name>` when the required first userspace payload is missing.
- `core/kernel/src/init/init_bootstrap.c` still emits `USER_INIT:*` and `BOOT_RUNTIME: STABLE` from kernel code when no init module is found.
- `core/kernel/src/kernel_boot.c` still emits the complete userspace success sequence on 32-bit ARM/RISC-V before entering the normal init loader path.
- `quality/contracts/boot/headless_boot_contract.yaml` still treats those user/runtime strings as plain substring requirements, so kernel-originated strings can satisfy the contract.
- `tools/run/runner_qemu.py` still records `online_cpu_count` from requested configuration rather than observed runtime evidence.

## Recommended first task

Start with a narrow **Gate 1A: fail-closed packaging** change.

Rationale:

1. It is the smallest coherent P0 fix.
2. It is tool-layer only, so it does not require changing the syscall ABI, scheduler, VM, SMP, or architecture entry paths.
3. It removes the easiest way to manufacture apparently valid boot evidence.
4. It creates an executable negative test: packaging without a produced `services/init`/`rt-supervisor` must fail non-zero.

Expected scope:

- `tools/package/packager.py`
- `quality/tests/tools/` or an equivalent focused host test for packager failure
- Documentation note if the package CLI behavior changes

Non-goals for the first patch:

- Do not introduce the boot nonce yet.
- Do not rewire FDT or Multiboot handoff yet.
- Do not change secure-boot naming or policy yet.
- Do not implement transactional ELF loading yet.

## Next tasks after Gate 1A

1. **Gate 1B: remove kernel-originated userspace success markers.** Missing init must emit a hard failure such as `BOOT_FAIL: INIT_MODULE_MISSING` and must not print `USER_INIT:*`.
2. **Gate 1C: runner/contract nonce proof.** Add a kernel boot nonce event, pass it through startup ABI, require `services/init` to report `USER_INIT_PROOF:<nonce>`, and make the runner correlate it.
3. **Gate 2: canonical module handoff.** Wire canonical FDT/Multiboot module parsing into live ARM64, RISC-V64, and x86 paths.
4. **Then address verified boot, transactional ELF loading, multikernel transaction correctness, coordinated reboot, SMP evidence, and BIDL cleanup.**

## Why not start elsewhere?

- Secure boot, transactional loading, reboot, and multikernel transaction fixes are important but are larger cross-subsystem changes. They should not be attempted while package and log evidence can still be fabricated.
- Canonical module handoff is the next best task, but it spans architecture entry code and boot adapters; it is safer after packaging can no longer hide missing payloads.
- SMP and BIDL issues are P1 relative to boot truth because they do not directly close the fabricated-userspace-success path.
