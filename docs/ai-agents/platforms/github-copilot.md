---
title: GitHub Copilot Runtime Notes
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
# GitHub Copilot Runtime Notes

## Recommended Inputs

- `CONTRIBUTING.md`
- `docs/ai-agents/standards/*`
- Inline task prompts with acceptance criteria

## Best Practices

- Provide explicit file paths and expected diff shape in prompts.
- Add guardrails in issue templates and PR templates.
- Require quality/tests/checks in PR automation.

## Workflow Suggestion

Use Copilot for code drafting; use repository guardrails and CI gates for merge enforcement.
