# Bharat-OS Claude Code & Claude Dev Guide

This repository contains **Bharat-OS**, a verification-oriented, capability-based microkernel evolving through per-core to distributed ownership.

Authoritative repository constitution: [AGENTS.md](AGENTS.md).

## Canonical Skills for Claude

Claude automatically discovers project skills in `.claude/skills/` (mirrored from `.agents/skills/` via `python3 tools/agents/sync_skills.py`).

| Area | Canonical Skill |
|---|---|
| Master Task Routing | `.claude/skills/bharat-os-task-router/SKILL.md` |
| Kernel, HAL & Drivers | `.claude/skills/bharat-os-kernel-architecture/SKILL.md` |
| Scheduler, TLB, Cross-Core | `.claude/skills/bharat-os-distributed-kernel/SKILL.md` |
| VM, PMM, MMU, MPU, DMA | `.claude/skills/bharat-os-memory-model/SKILL.md` |
| Capabilities & Security | `.claude/skills/bharat-os-capability-security/SKILL.md` |
| Services & Supervisors | `.claude/skills/bharat-os-service-runtime/SKILL.md` |
| Display, GUI & Input Stream | `.claude/skills/bharat-os-gui-experience/SKILL.md` |
| Host Testing & Mocks | `.claude/skills/bharat-os-host-testing/SKILL.md` |
| CMake & Kernel Config | `.claude/skills/bharat-os-kernel-config/SKILL.md` |
| Syscall ABI & UAPI | `.claude/skills/bharat-os-syscall-abi/SKILL.md` |
| Linux & Android Shims | `.claude/skills/bharat-os-personality/SKILL.md` |
| Verification & Matrix | `.claude/skills/bharat-os-validation/SKILL.md` |
| Machine-Readable Status | `.claude/skills/bharat-os-runtime-truth/SKILL.md` |
| Architecture Docs & ADRs | `.claude/skills/bharat-os-architecture-docs/SKILL.md` |
| PR & Code Review | `.claude/skills/bharat-os-review/SKILL.md` |

## Fast Context Pack

Before analyzing large swaths of the codebase, generate a compact context pack:
```bash
python3 tools/agents/context_pack.py --area <area-name> --format compact
```

## Mandatory Verification Commands

Before completing any task or PR:
```bash
# Run automated agent verifier (linters, ABI, changed-file gates)
python3 tools/agents/verify.py --changed-files <changed_files>

# Run 5-architecture QEMU smoke matrix
python3 tools/run_qemu_matrix.py --headless --smoke --all-arch
```

## Core Invariants & Prohibitions
1. **Kernel = mechanisms only; Services = policy.** Never place policy or drivers inside `core/kernel/`.
2. **Every resource is capability-mediated.** Deny by default, fail closed.
3. **Core-local ownership.** No shared mutable cross-core state without message-based ownership protocol.
4. **No raw pointers crossing wire boundaries.** Wire structs must be fixed-width with compile-time assertions.
5. **No spin-count waits.** Monotonic HAL time (`bh_ktime_now()`) is required for deadlines.
6. **No committing generated files.** `bharat_config.h` and generated syscall tables stay in `build/`.
