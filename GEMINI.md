# Bharat-OS Gemini Context

Read and obey `AGENTS.md` at the repository root before planning or editing. It is the canonical repository constitution.

For component-specific work, also read the nearest scoped `AGENTS.md`, relevant files under `docs/architecture/` and `docs/adr/`, and the applicable contract under `interface/contracts/`.

Key requirements:

- Bharat-OS is a capability-based microkernel evolving toward per-core and distributed-kernel ownership.
- Keep mechanisms in the kernel and policy in services.
- No pointer crossing or direct remote mutation across core/message boundaries.
- New public kernel APIs use `bh_*`; public constants/macros use `BH_*`.
- Support MMU, MMU-Lite, and MPU paths; fail closed when unsupported.
- Never edit or commit generated `bharat_config.h`; modify `core/kernel/include/bharat_config.h.in`.
- Do not modify `ext/` or `external/` unless explicitly requested.
- Follow the mandatory five-target build and QEMU matrix rules in `AGENTS.md`.
- Report unavailable targets or unsupported runner flags as BLOCKED, never as PASS.

Do not duplicate or reinterpret the full rules here. `AGENTS.md` remains authoritative.
