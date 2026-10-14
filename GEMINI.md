# Bharat-OS Gemini Context

Read and obey [AGENTS.md](AGENTS.md) at the repository root before planning or editing. It is the canonical repository constitution.

For subsystem-specific work, use the task router and load canonical skills from `.agents/skills/`:

```bash
# Query compact context pack for the target subsystem
python3 tools/agents/context_pack.py --area <subsystem-name> --format compact
```

Key requirements:
- **Architecture**: Bharat-OS is a verification-oriented capability microkernel evolving toward per-core and distributed ownership.
- **Placement**: Keep mechanisms in `core/kernel/` and policy in `core/services/`.
- **Wire boundary**: No pointer crossing or direct remote mutation across core/message boundaries. Fixed-width structures with compile-time assertions only.
- **Timing**: No spin-count waits. Use monotonic HAL deadlines (`bh_ktime_now()`).
- **Memory**: Support MMU, MMU-Lite, and MPU paths; fail closed when unsupported.
- **Config**: Never edit or commit generated `bharat_config.h`; modify `core/kernel/include/bharat_config.h.in`.
- **Validation**: Ensure QEMU system emulators (`x86_64`, `aarch64`, `riscv64`, `arm`, `riscv32`) are installed for all 5 target architectures. Run `python3 tools/agents/verify.py` and `python3 tools/run_qemu_matrix.py --headless --smoke --all-arch`.
- **Honesty**: Report unavailable targets or missing toolchains as BLOCKED, never as PASS.
