---
title: Personalities Documentation Index
status: active
owner: Architecture Team
version: 1.1
last_updated: "2026-04-22"
---

# Personalities Documentation (Consolidated)

This folder is the canonical architecture location for personality design and planning.

## Core documents

- `personality-layer.md`: architecture model and layering rules.
- `personality-performance-contract.md`: mandatory performance and KPI constraints.
- `roadmap.md`: phased delivery roadmap.
- `zero-translation-roadmap.md`: implementation roadmap for near-zero translation overhead using modern native primitives.
- `multi-arch-personality-roadmap.md`: cross-ISA execution roadmap for Linux and Android personalities on x86_64, arm64, and riscv64.
- `production-grade-personality-plan.md`: executable implementation backlog for production-grade Linux + Android personalities with no-translation-tax gates.

## Personality-specific docs

- `linux/` and `linux-*.md`
- `android/` and `android-*.md`
- `native-personality.md`

## Consolidation policy

- New personality architecture and roadmap docs should be created under this folder.
- Legacy references outside this folder can remain for historical ADR context, but active implementation guidance should link back here.
