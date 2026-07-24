---
title: Google/Jules-Style Agent Notes
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
# Google/Jules-Style Agent Notes

## Recommended Inputs

- Scoped task definition (one feature/fix per request)
- Repository-level standards and guardrails
- Build/test commands for touched components

## Best Practices

- Keep prompts deterministic and acceptance-test oriented.
- Separate "analysis" from "code generation" steps.
- Require output to include changed files and validations.

## Workflow Suggestion

Adopt a task card format: objective, constraints, files in scope, checks, and definition of done.
