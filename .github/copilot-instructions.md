# GitHub Copilot Instructions for Bharat-OS

Use the repository root `AGENTS.md` as the authoritative instruction set for code generation, code review, fixes, and pull-request work.

Always apply these repository-wide rules:

- Preserve the verification-oriented capability microkernel architecture and its evolution toward per-core and distributed ownership.
- Keep kernel mechanisms small; place policy and orchestration in services.
- Do not introduce global mutable kernel state, direct remote-core mutation, unbounded waits, or pointer-bearing IPC/uRPC messages.
- New public kernel functions/types use `bh_*`; constants/macros/enums use `BH_*`.
- Preserve the syscall trust boundary in `interface/contracts/abi/native_syscalls.json`.
- Never hand-edit generated ABI outputs or generated `bharat_config.h`.
- Never modify `ext/` or `external/` unless explicitly requested.
- Add tests for changed behavior and update architecture/contract documentation when behavior changes.
- Use the exact validation and completion-report requirements in `AGENTS.md`.
- Never describe skipped, unavailable, or unexecuted validation as passing.

Use path-specific files under `.github/instructions/` for additional scoped rules.
