---
title: AI Scheduler Overview
status: Draft
owner: Documentation Working Group
last_updated: 2026-04-25
tags:
  - docs
  - architecture
  - kernel
see_also:
  - README.md
---
# AI Scheduler Overview

## Design intent

Bharat-OS keeps scheduling **mechanism** in kernel and treats AI inputs as **bounded hints**. The kernel never executes unbounded ML policy loops in the hot path.

## Code path summary

### Core telemetry and context

- Telemetry/context structures: `core/kernel/include/sched/ai_sched.h`
- Telemetry update + fallback sampling + simple complexity prediction: `core/kernel/src/sched/ai_sched.c`

`ai_sched_collect_sample(...)` first tries `ai_sched_arch_sample_pmc(...)` (weak hook, arch-overridable). If PMCs are unavailable, it uses deterministic fallback estimation from timeslice/profile scaling.

### Suggestion ingestion and execution boundary

- Queue ingress: `sched_enqueue_ai_suggestion(...)` in `core/kernel/src/sched/sched_ai_ingest.c`
- Queue drain: `sched_process_pending_ai_suggestions(...)` in `core/kernel/src/sched/sched_ai_ingest.c`
- Action execution: `sched_ai_apply_suggestion(...)` path in scheduler core modules

Suggestions are queued and processed from scheduler/tick paths, not executed directly in receive context.

## Supported suggestion actions

Defined in `ai_action_t`:

- `AI_ACTION_MIGRATE_TASK`
- `AI_ACTION_ADJUST_PRIORITY`
- `AI_ACTION_THROTTLE_CORE`
- `AI_ACTION_KILL_TASK`

## Safety properties (current)

- Bounded ring-buffer style pending queue (`SCHED_MAX_PENDING_SUGGESTIONS`).
- Null and full-queue rejection in enqueue path.
- Scheduler lifecycle/state ownership remains in core scheduler logic.

## Non-goals (current)

- No in-kernel heavy ML runtime.
- No direct policy replacement of scheduler class logic by AI.
