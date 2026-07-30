---
applyTo: "core/kernel/**/*.{c,h,S},core/arch/**/*.{c,h,S},core/hal/**/*.{c,h,S}"
---

# Bharat-OS Kernel and Low-Level Instructions

- Read root `AGENTS.md` and the nearest scoped `AGENTS.md` before editing.
- Keep kernel code mechanism-only and verification-friendly.
- New public functions/types use `bh_*`; public constants/macros/enums use `BH_*`.
- Document ownership, lock ordering, memory ordering, lifecycle states, and cross-core protocol invariants.
- Do not pass pointers or native-width layout-dependent fields through IPC/uRPC/wire structures.
- Use fixed-width versioned messages and static layout assertions.
- Do not hold locks while waiting for remote completion.
- Use monotonic HAL time and bounded retries; implement replay/idempotence where retries occur.
- Consider MMU, MMU-Lite, and MPU backends for memory-protection changes.
- Add focused regression, negative, concurrency, and partial-failure tests.
- Run the mandatory architecture and QEMU gates from root `AGENTS.md` before completion.
