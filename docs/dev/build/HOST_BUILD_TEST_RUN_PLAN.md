---
title: Build/Test/Run Modernization Plan
status: Draft
owner: Documentation Working Group
last_updated: 2026-04-25
tags:
  - docs
  - dev
  - build
see_also:
  - README.md
---
# Build/Test/Run Modernization Plan

This plan tracks migration to a YAML-target-first, preset-driven build workflow.

## Phase 1 — Documentation migration (completed)

- Move docs from `docs/build/` to `docs/dev/build/`.
- Align docs to authoritative CLI in `tools/build.py`.
- Document legacy compatibility flags (`--run`, `--build`, `--all`) as non-authoritative.

## Phase 2 — CI command normalization

- Replace legacy positional invocations in CI with explicit subcommands.
- Require `--target` or `--target-yaml` in all scripts.
- Standardize smoke jobs around headless target set.

## Phase 3 — Test preset standardization

- Use named test presets from `CMakePresets.json` (`host-test`, `host-test-asan`, `host-test-valgrind`, `ci-smoke`).
- Remove old doc references to non-canonical test preset names.

## Phase 4 — YAML target expansion

- Add/maintain target YAMLs for profile families and execution profiles.
- Keep per-target run/flash/debug contracts inside YAML to avoid script drift.

## Phase 5 — Gate policy

- Per-PR: host test preset + at least one architecture build.
- Nightly: multi-arch headless `all` workflows + sanitizer/valgrind track.
- Release: flash/debug manifest validation for hardware-enabled targets.

## Exit criteria

- No CI job depends on positional legacy CLI mode.
- Headless YAML targets are the default in docs and CI recipes.
- Test preset and target matrix usage is stable across Linux + Windows.
