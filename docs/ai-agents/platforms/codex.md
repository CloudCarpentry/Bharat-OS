---
title: Codex Runtime Notes
status: Draft
owner: Documentation Working Group
last_updated: 2026-04-25
tags:
  - docs
  - ai-agents
  - platforms
see_also:
  - README.md
---
# Codex Runtime Notes

## Recommended Files

- Root `AGENTS.md` for project-wide policy.
- Nested `AGENTS.md` for directory-specific behavior.
- Skill folders with `SKILL.md` for specialized tasks.

## Best Practices

- Keep instruction precedence explicit (system/developer/user > AGENTS).
- Provide clear test command expectations per area.
- Use concise final summaries with file-level references.

## Integration Tip

Create links from root docs to `docs/ai-agents/` so maintainers can quickly discover governance material.
