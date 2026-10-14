---
title: AI Agent & Skill Documentation Kit
status: Active
owner: Documentation Working Group
last_updated: 2026-10-14
tags:
  - docs
  - ai-agents
see_also:
  - README.md
  - AGENTS.md
---
# AI Agent & Skill Documentation Kit

This folder provides the cross-platform integration and documentation structure for AI coding agents and IDE assistants working on Bharat-OS:

- **Google Antigravity**: Native `.agents/skills/<skill>/SKILL.md` and `.agents/agents.md`
- **Google Jules**: Automatic root [AGENTS.md](../../AGENTS.md) ingestion and task router
- **GitHub Copilot**: [.github/copilot-instructions.md](../../.github/copilot-instructions.md) and `.github/skills/`
- **Claude (Claude Code / Claude Dev)**: [CLAUDE.md](../../CLAUDE.md) and `.claude/skills/`
- **Cline / Cursor**: [/.clinerules](../../.clinerules), [/.cursorrules](../../.cursorrules)

## Canonical Source of Truth

The **single authoritative source of truth** for skills is:
- **Skills Directory**: `/.agents/skills/<skill_name>/SKILL.md`

Platform skill directories are automatically mirrored and synchronized via:
```bash
python3 tools/agents/sync_skills.py --sync
```

Validate agent configurations and skill frontmatter anytime with:
```bash
python3 tools/agents/check_agent_config.py
```

## Standardized Canonical Skills (15)

1. `bharat-os-task-router`: Classifies task, queries `context_map.yaml`, selects minimal skills.
2. `bharat-os-kernel-architecture`: Kernel mechanisms vs service policy, layer placement.
3. `bharat-os-distributed-kernel`: Cross-core, TLB shootdown, fixed-width wire structs, monotonic deadlines.
4. `bharat-os-memory-model`: MMU_FULL, MMU_LITE, MPU/PMP, pcache magazines, no recursive memops.
5. `bharat-os-capability-security`: Capability mediation, usercopy validation phase, revocation.
6. `bharat-os-service-runtime`: Production authority adapters, real capability endpoints, runtime markers.
7. `bharat-os-gui-experience`: Display tiers (ADR-035), capability input stream (ADR-034), GUI smoke matrix.
8. `bharat-os-host-testing`: Fast CTest suites, mock HAL, synthetic core concurrency (`BHARAT_HOST_TEST=1`).
9. `bharat-os-kernel-config`: `bharat_config.h.in` template authority, profile matrix checks.
10. `bharat-os-syscall-abi`: Native syscall manifest, ABI lock, usercopy bounds.
11. `bharat-os-personality`: Linux, Android, POSIX compatibility shims strictly outside kernel.
12. `bharat-os-validation`: 5-target QEMU matrix, `verify.py`, empirical evidence rules.
13. `bharat-os-runtime-truth`: `implementation_maturity.json` machine-readable status.
14. `bharat-os-architecture-docs`: ADR lifecycle, documentation hierarchy, CONTRACTS.md sync.
15. `bharat-os-review`: Pre-merge architectural hygiene and security checklist.

## Verification & Qualification Suite

Run verification through the unified agent verifier `tools/agents/verify.py`:
```bash
# 1. Fast static linters & contract checks
python3 tools/agents/verify.py --linters-only

# 2. Host unit & integration test suites
python3 tools/agents/verify.py --host-tests

# 3. Canonical 5-Architecture Headless QEMU Smoke Matrix
python3 tools/agents/verify.py --matrix-only

# 4. Multi-Architecture GUI Smoke Matrix (Desktop & Mobile)
python3 tools/agents/verify.py --gui-smoke

# 5. Device Profile Qualifications (desktop, mobile, automotive, rtos, drone)
python3 tools/agents/verify.py --profile-smoke mobile
python3 tools/agents/verify.py --profile-smoke automotive
python3 tools/agents/verify.py --profile-smoke drone
```

Autonomous agents save substantial context tokens by querying the compact context pack instead of scanning the tree:
```bash
python3 tools/agents/context_pack.py --area <area-name> --format compact
```

## Folder Structure

```text
.agents/
├── agents.md               # Antigravity team roles mapped to skills
├── skills/                 # CANONICAL SKILL DEFINITIONS (Single Source of Truth)
│   ├── bharat-os-task-router/
│   ├── bharat-os-kernel-architecture/
│   ├── bharat-os-distributed-kernel/
│   ├── bharat-os-memory-model/
│   ├── bharat-os-capability-security/
│   ├── bharat-os-service-runtime/
│   ├── bharat-os-kernel-config/
│   ├── bharat-os-syscall-abi/
│   ├── bharat-os-personality/
│   ├── bharat-os-validation/
│   ├── bharat-os-runtime-truth/
│   └── bharat-os-review/
└── workflows/              # Reusable agent workflows

tools/agents/
├── check_agent_config.py   # Linter for skill frontmatter, targets, and sync
├── context_map.yaml        # Machine-readable subsystem metadata
├── context_pack.py         # CLI context generator for agents
├── sync_skills.py          # Platform skill mirror (.claude/ & .github/)
└── verify.py               # Unified agent verification front-end
```
