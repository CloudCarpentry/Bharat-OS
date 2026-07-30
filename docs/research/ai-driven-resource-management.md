---
title: Ai Driven Resource Management
status: Draft
owner: Documentation Working Group
last_updated: 2026-04-25
tags:
  - docs
  - research
see_also:
  - README.md
---
## 🧠 AI-Driven Resource Management

Detailed mapping is documented in [`docs/architecture/device-profiles-and-use-cases.md`](docs/architecture/device-profiles-and-use-cases.md).

## AI Features & Roadmap

Bharat-OS keeps AI policy outside ring-0 while exposing bounded kernel mechanisms:

### Implemented baseline

- Kernel-side telemetry collection hooks and bounded AI suggestion queueing.
- Scheduler action handling for migrate/priority/throttle suggestion types.
- Capability-guarded governor control-plane endpoint.
- Architecture/profile-neutral telemetry plugin contract (with fallback behavior when PMCs are unavailable).

#### Scheduler & AI Hooks Architecture

```mermaid
%%{init: {'theme': 'base', 'themeVariables': { 'primaryColor': '#ccffdd', 'edgeLabelBackground':'#ffffff'}}}%%
graph TD
    subgraph UserSpace [User-Space Policy]
        AIGov[AI Governor]
        SystemProfile[Profile-Aware Heuristics - Tier A/B/C]
        TelemetryCollection[Telemetry Ingestion]
    end

    subgraph Microkernel [Bharat-OS Microkernel Ring-0]
        Sched[Tick-driven Scheduler]
        Telemetry[Kernel Telemetry Collection - Cycles, Instructions, CPI]
        AIHooks[Bounded AI Suggestion Queue - Migrate, Priority, Throttle]
        Fallback[Deterministic Fallback - Round Robin]
    end

    subgraph Hardware [Hardware Abstraction Layer]
        PMCs[Hardware PMCs - Architecture Specific]
        Timer[Generic Timer Tick]
    end

    Timer -->|Tick Event| Sched
    PMCs -->|Sample Counters| Telemetry

    Sched --> Telemetry
    Telemetry -->|Expose Metrics| TelemetryCollection

    SystemProfile --> AIGov
    TelemetryCollection --> AIGov

    AIGov -->|Submit AI Suggestions| AIHooks
    AIHooks -->|Validate Bounds| Sched

    Sched -->|If PMCs / AI unavailable| Fallback
    Sched -->|Context Switch| Hardware
```

### Roadmap

- Better telemetry quality (hardware PMC integrations per architecture).
- Per-core runqueues + richer migration policy under SMP load.
- Safety/verification hardening for AI-driven scheduling decisions.
- Clearer user-space governor lifecycle, observability, and audit reporting.

See [`docs/architecture/ai-scheduler-status-and-roadmap.md`](docs/architecture/ai-scheduler-status-and-roadmap.md) and [`docs/adr/ADR-008-ai-scheduler-plugin-contract.md`](docs/adr/ADR-008-ai-scheduler-plugin-contract.md).
