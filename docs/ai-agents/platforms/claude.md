---
title: Claude Runtime Notes
status: Active
owner: Documentation Working Group
last_updated: 2026-10-14
tags:
  - docs
  - ai-agents
  - platforms
see_also:
  - README.md
  - CLAUDE.md
  - AGENTS.md
---
# Claude Runtime Notes (Claude Code & Claude Dev)

Claude natively loads skills on demand from `.claude/skills/<name>/SKILL.md` and reads `CLAUDE.md` at the repository root.

## Configuration & Synchronization

1. Canonical skill source: `.agents/skills/`.
2. Synchronize to `.claude/skills/`:
   ```bash
   python3 tools/agents/sync_skills.py --sync
   ```
3. Check synchronization:
   ```bash
   python3 tools/agents/sync_skills.py --check
   ```

## Workflow for Claude Agents

1. **Context pack query**:
   Run `python3 tools/agents/context_pack.py --area <subsystem>` to retrieve the compact 40-line summary.
2. **Skill discovery**:
   Claude loads the relevant canonical skill from `.claude/skills/` on demand.
3. **Verification**:
   Run `python3 tools/agents/verify.py --changed-files <...>` before submitting changes.
