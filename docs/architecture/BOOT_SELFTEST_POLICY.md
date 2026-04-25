---
title: Bharat-OS Boot Self-Test Policy
status: Draft
owner: Documentation Working Group
last_updated: 2026-04-25
tags:
  - docs
  - architecture
see_also:
  - README.md
---
# Bharat-OS Boot Self-Test Policy

## Purpose
This document defines the staged boot self-test policy for Bharat-OS.

The purpose of boot self-tests is to validate essential kernel and platform health during boot without turning every boot into a full diagnostic or stress session.

Boot self-test policy separates:
- mandatory production-safe sanity checks
- quick diagnostic smoke tests
- extended or manufacturing-oriented validation
- benchmark-only measurements

## Design goals
- keep normal boot deterministic and fast
- support multiple architectures and boards
- respect hardware capability differences
- integrate with boot phases
- reuse existing kernel test infrastructure
- allow richer diagnostics in diagnostic and manufacturing modes

## Core concepts

### Boot test stage
Boot self-tests are associated with a stage:
- early
- security
- memory
- platform
- runtime

A test should only run when the system has reached the stage required for that test.

### Boot test policy class
Each boot-relevant test belongs to a policy class:
- mandatory
- quick
- extended
- manufacturing
- benchmark-only

### Capability requirements
A test may require specific capabilities such as:
- PMM
- VMM
- MMU
- SMP
- timer
- UART
- framebuffer
- scheduler
- IPC

If required capabilities are unavailable, the test is skipped rather than failed.

### Fatal vs non-fatal
Mandatory sanity checks may be fatal on failure.
Quick, extended, manufacturing, and benchmark-oriented checks are non-fatal by default unless explicitly marked otherwise.

## Policy by boot mode

### Normal
Runs mandatory checks only.

### Diagnostic
Runs mandatory and quick checks.

### Recovery
Runs only the smallest safe mandatory checks.

### Manufacturing
Runs mandatory, quick, and manufacturing-tagged checks.

### Benchmark
Runs mandatory checks plus benchmark-only tests when explicitly selected.

### Legacy bring-up
Preserves migration-time bring-up behavior while remaining more structured than an unrestricted generic boot test sweep.

## Stage integration
Boot self-tests are integrated with boot phases:
1. early
2. security
3. memory
4. platform
5. runtime

This ensures that checks run only when their dependencies are valid.

## First-wave coverage
Initial staged boot self-test coverage focuses on:
- boot handoff sanity
- PMM
- page tables
- VMM
- TLB basics
- timer and interrupt basics
- scheduler bootstrap
- console basics

## Future work
Future work may include:
- deeper stress validation
- shell-triggered diagnostics
- board-specific manufacturing plans
- richer self-test reporting

## Boot Validation Phase (Pre-Runtime)
A new `boot_validate_all()` layer executes immediately on `kernel_main_common()`, explicitly parsing the `boot_info_t` array for overlapping memory, missing initrds, or security combination failures before any true selftests run.
