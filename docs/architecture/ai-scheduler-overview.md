# AI Scheduler Overview

This document explains Bharat-OS's current AI-guided scheduling path and how it stays portable across architectures.

## Goals

- preserve deterministic core scheduler behavior
- enrich scheduling with bounded AI suggestions
- keep architecture-specific telemetry optional and pluggable

## Current components

- Kernel scheduler telemetry and prediction logic: `kernel/src/ai_sched.c`
- Architecture-neutral scheduler contract: `kernel/include/advanced/ai_sched.h`
- User-space governor seed implementation: `subsys/src/ai_governor.c`
- Plugin contract ADR: `docs/decisions/ADR-008-ai-scheduler-plugin-contract.md`

## Telemetry model

Per-thread telemetry maintained by the scheduler includes:

- total cycle estimates
- instruction count estimates
- CPI-style derived metrics
- complexity prediction context

When architecture PMCs are available, profile/arch hooks can sample hardware counters. When PMCs are unavailable, the scheduler uses deterministic approximations from timeslice usage, run-queue context, and switch behavior.

## Plugin architecture (ADR-008)

The scheduler core depends only on arch-neutral helper APIs and bounded callbacks. Architecture/profile plugins can provide:

- PMC sampling overrides
- scaling-factor policies by profile (`RTOS`, `EDGE`, `DESKTOP`)
- optional heuristics for migration/priority/throttle suggestions

This keeps the main scheduler path testable and portable while allowing architecture-specific optimization.

## Execution flow summary

1. Timer tick enters scheduler path.
2. Telemetry update runs for active threads.
3. Pending AI suggestions are processed with bounded queue logic.
4. Scheduler applies accepted actions (migrate, reprioritize, throttle).
5. Dispatch selects next runnable thread.

## Planned extensions

- user-space AI governor policies feeding scheduler hints
- profile-tuned heuristics for mobile/edge/cloud tiers
- accelerator-aware placement (NPU/GPU/DSP workloads)
- stronger power/thermal coordination hooks
