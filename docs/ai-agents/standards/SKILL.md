---
title: Skill Specification (Cross-Platform)
status: Draft
owner: Documentation Working Group
last_updated: 2026-04-25
tags:
  - docs
  - ai-agents
  - standards
see_also:
  - README.md
---
# Skill Specification (Cross-Platform)

A skill is a reusable package of instructions + optional scripts/templates that an agent can invoke for a specific class of tasks.

## Skill Metadata

Each skill should define:

- Name
- Purpose
- Inputs required
- Outputs produced
- Constraints/safety notes
- Success criteria

## Suggested Skill Layout

```text
<skill-root>/
├── SKILL.md
├── scripts/        # Optional helper scripts
├── templates/      # Optional markdown/code templates
└── references/     # Optional focused references
```

## Authoring Rules

- Keep instructions procedural and short.
- Prefer deterministic commands over subjective wording.
- Include "when not to use" section.
- Include fallback behavior when dependencies are missing.

## Runtime Rules

- Only load files needed for the current task.
- Resolve relative paths from the skill directory first.
- Avoid bulk-loading entire references folders.
- If multiple skills apply, declare ordering and rationale.

## Example Skill Lifecycle

1. Detect trigger phrase or matching intent.
2. Open `SKILL.md`.
3. Collect required inputs.
4. Execute workflow steps.
5. Validate outputs against success criteria.
6. Return concise outcome with artifacts.
