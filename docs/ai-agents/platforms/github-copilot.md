---
title: GitHub Copilot Runtime Notes
status: Active
owner: Documentation Working Group
last_updated: 2026-10-14
tags:
  - docs
  - ai-agents
  - platforms
see_also:
  - README.md
  - AGENTS.md
  - .github/copilot-instructions.md
---
# GitHub Copilot Runtime Notes

GitHub Copilot is guided by `.github/copilot-instructions.md` and discovers project skills in `.github/skills/` (and `.agents/skills/`).

## Setup & Skills Synchronization

1. GitHub Copilot reads repository-wide instructions from `.github/copilot-instructions.md` and scoped files in `.github/instructions/`.
2. Skills are mirrored from `.agents/skills/` into `.github/skills/` using:
   ```bash
   python3 tools/agents/sync_skills.py --sync
   ```

## Workflow for GitHub Copilot

1. Use `python3 tools/agents/context_pack.py --area <area-name>` to get compact subsystem boundaries and skill references.
2. Read the referenced domain skill in `.github/skills/<skill>/SKILL.md`.
3. Follow the 4-pillar constitution in root `AGENTS.md`.
4. Validate changes using `python3 tools/agents/verify.py --changed-files <...>`.
