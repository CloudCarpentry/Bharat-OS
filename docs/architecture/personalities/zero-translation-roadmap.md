---
title: Personality Zero-Translation Roadmap
status: active
owner: Architecture Team
version: 1.0
last_updated: "2026-04-21"
tags: ["architecture", "personalities", "performance", "compatibility"]
---

# Personality Zero-Translation Roadmap

## Goal

Ensure Linux, Android, and future personalities remain **fully functional and performance-grade**, avoiding the "works but slow" trap from repeated translation layers.

## 1. Design rules

1. **Boundary-only translation**: translate once at personality ABI ingress, not across every internal subsystem.
2. **No text translation on hot path**: avoid string parsing/matching in syscall, IPC, memory, scheduling, and binder/epoll-like loops.
3. **Native primitive reuse**: all personalities should reuse the same kernel primitives and only adapt semantics.
4. **Fast-path determinism**: hot-path mapping must be table-driven and cacheable.

## 2. Modern native primitives used

The current primitive set that personalities must target:

- **Intent execution**: `intent_set/get` for scheduling intent attachment.
- **Memory class allocation**: class-tagged allocation path for policy-aware memory behavior.
- **Fault domains**: explicit containment/restart boundaries.

These are additive; default legacy paths must continue without forced remapping.

## 3. Architecture pattern per personality

### Linux
- Map `sched_setscheduler`/affinity/cgroup-style hints to intent metadata only when feature-flagged.
- Map selected DMA/packet-intensive paths to memory classes.
- Map service groups/process trees to fault domains for controlled restarts.

### Android
- Map task profiles/foreground classes to intent metadata.
- Map media/ML/sensor memory flows to class-tagged allocation.
- Map critical service clusters to fault domains with supervisor orchestration.

## 4. Rollout plan

### Stage 0: Baseline parity
- Keep mappings disabled by default.
- Validate no regression in existing compatibility paths.

### Stage 1: Opt-in feature flags
- Enable mapping per subsystem (scheduler first, memory second, fault policy third).
- Expose policy toggles through profile config.

### Stage 2: Zero-translation maturity
- Replace remaining string-heavy pathways with compact IDs/enums.
- Add per-thread/per-domain mapping caches.
- Remove duplicate translation between personality and services.

### Stage 3: Default-on where proven
- Enable only after KPI gates pass in CI/perf labs.

## 5. KPI gates

- Null syscall latency delta stays within agreed budget vs baseline.
- `mmap`/alloc class overhead remains within low single-digit range.
- `epoll`/wait wakeup path avoids per-event translation churn.
- Binder-like and shared-memory flows maintain zero-copy behavior.
- Fault-domain attach/create overhead bounded and non-disruptive.

## 6. Primitive quality improvements required

To maximize personality benefit, primitive implementations should mature from v1 stubs to production paths:

- Intent: persist per-thread metadata and fast retrieval.
- Mem class: replace placeholder handle flow with real mapped allocation + accounting.
- Fault domain: persist domain state and enforce attach/lifecycle semantics.

## 7. Non-negotiable guardrails

- No ABI renumbering or replacement of legacy compatibility interfaces.
- Additive UAPI only, with versioned structs and reserved fields.
- Degrade behavior explicit per profile when semantics cannot be fully honored.
