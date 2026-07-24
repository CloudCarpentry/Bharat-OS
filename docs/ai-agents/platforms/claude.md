---
title: Claude Runtime Notes
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
# Claude Runtime Notes

## Recommended Inputs

- Clear objective with constraints and non-goals
- Existing architecture docs for impacted subsystem
- Agent standards + guardrails docs

## Best Practices

- Ask for stepwise plan + implementation + verification in one response cycle.
- Request explicit assumptions and unknowns.
- Require changed-file summaries and command evidence.

## Workflow Suggestion

Use Claude for architecture-level reasoning and draft docs; validate with project-native tests before merge.
