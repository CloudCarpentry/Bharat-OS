---
title: Antigravity IDE Integration Guide
status: Draft
owner: Documentation Working Group
last_updated: 2026-04-25
tags:
  - docs
  - ai-agents
  - ide
see_also:
  - README.md
---
# Antigravity IDE Integration Guide

## Recommended Setup

- Surface `docs/ai-agents/standards/` in workspace docs panel.
- Configure reusable task presets (build/test/docs checks).
- Keep instruction files near active code for contextual retrieval.

## Suggested Team Rules

- Define one canonical prompt template for engineering tasks.
- Require final output to include changed files and checks.
- Enforce guardrails via review checklist before merge.

## Prompting Pattern

Provide: objective, constraints, scope, validation commands, and risks to avoid.
