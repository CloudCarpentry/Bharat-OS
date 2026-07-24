---
title: VS Code Integration Guide
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
# VS Code Integration Guide

## Recommended Setup

- Add workspace tasks for build/test/lint.
- Keep agent docs discoverable via repo README links.
- Use problem matchers so issues are navigable by file/line.

## Suggested Workspace Conventions

- `.vscode/tasks.json`: standard commands
- `.vscode/settings.json`: formatting + lint defaults
- `.vscode/extensions.json`: recommended extension list

## Prompting Pattern

When using assistant extensions, include:

- Goal
- Scope (files/directories)
- Constraints
- Checks to run
- Expected output format
