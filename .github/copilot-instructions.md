# GitHub Copilot Instructions for Bharat-OS

Use the repository root [AGENTS.md](../AGENTS.md) as the authoritative constitution for all code generation, reviews, and fixes.

## Canonical Skills for GitHub Copilot

Copilot discovers project skills in `.github/skills/` (automatically synced from `.agents/skills/`):

- **Master Task Router**: `.github/skills/bharat-os-task-router/SKILL.md` (Always start here)
- **Kernel Architecture**: `.github/skills/bharat-os-kernel-architecture/SKILL.md` (Layer placement, mechanisms vs. policy)
- **Distributed Kernel**: `.github/skills/bharat-os-distributed-kernel/SKILL.md` (Scheduler, cross-core, wire structs, monotonic deadlines)
- **Memory Model**: `.github/skills/bharat-os-memory-model/SKILL.md` (MMU_FULL, MMU_LITE, MPU/PMP, pcache, no recursive memops)
- **Capability Security**: `.github/skills/bharat-os-capability-security/SKILL.md` (Validation, usercopy boundary, revocation)
- **Service Runtime**: `.github/skills/bharat-os-service-runtime/SKILL.md` (Production authority adapters, real endpoints)
- **GUI & Experience**: `.github/skills/bharat-os-gui-experience/SKILL.md` (Display tiers, input stream, GUI smoke)
- **Host Testing**: `.github/skills/bharat-os-host-testing/SKILL.md` (Fast CTest test suites, mock HAL)
- **Kernel Config**: `.github/skills/bharat-os-kernel-config/SKILL.md` (Template `bharat_config.h.in`, deterministic builds)
- **Syscall ABI**: `.github/skills/bharat-os-syscall-abi/SKILL.md` (ABI lock, `syscall_abi.py --check`)
- **Personality Layers**: `.github/skills/bharat-os-personality/SKILL.md` (Linux & Android compatibility shims outside kernel)
- **Validation**: `.github/skills/bharat-os-validation/SKILL.md` (5-target QEMU matrix)
- **Runtime Truth**: `.github/skills/bharat-os-runtime-truth/SKILL.md` (Machine-readable status in `implementation_maturity.json`)
- **Architecture Docs**: `.github/skills/bharat-os-architecture-docs/SKILL.md` (ADR lifecycle, CONTRACTS.md sync)
- **Review**: `.github/skills/bharat-os-review/SKILL.md` (Pre-merge checklist)

## Fast Subsystem Context Pack

Run this command to get a compact context summary (50-80 lines) rather than searching the repository:
```bash
python3 tools/agents/context_pack.py --area <area-name> --format compact
```

## Mandatory Guardrails

1. **Layer separation**: Mechanisms in `core/kernel/`, policy in `core/services/`, ISA in `core/arch/`, contracts in `core/hal/`.
2. **Never pass pointers across wire boundaries**: IPC/uRPC structures must be fixed-width with compile-time assertions.
3. **No spin-count remote waits**: Deadlines must use monotonic time (`bh_ktime_now()`).
4. **Never bypass capabilities**: Deny by default, fail closed.
5. **Never dereference user pointers directly**: Copy via fault-safe usercopy before inspecting fields or capability handles.
6. **Never commit generated files**: `bharat_config.h` and generated syscall headers stay in `build/`.
7. **Empirical evidence only**: Verify via `python3 tools/agents/verify.py` and `python3 tools/run_qemu_matrix.py --headless --smoke --all-arch`. Never report unexecuted checks as PASS.
