# services/init

## Purpose
The first user-space root supervisor, acting strictly as a **bootstrap coordinator**. It launches the core service graph, enforces initial startup order, acts as a health gate based on kernel-provided boot status, and handles early boot failure policy (like safe mode). It is designed to be profile-aware and deterministic.

**Crucially, `init` is NOT a permanent service supervisor, process tree owner, or a Linux-style systemd clone.** After early boot convergence and graph validation, `init` hands off long-lived lifecycle management, fault handling, and restart policies to `servicemgr` and `faultmgr`. It may then exit or become quiescent.

## Dependencies
- **May depend on:** `lib/runtime`, `lib/ipc`, `lib/cap`, standard C library headers.
- **Must not depend on:** other `services/*` (no service-to-service compile-time dependency), `subsys/*`, `ui/*`.
- **Must not use:** direct kernel-private headers or duplicate kernel self-test logic.

## Planned Public API
- Subsystem init hooks / normalized boot context contract (`init_boot_context.h`).
- Structured handoff protocol to `servicemgr`.
- Machine-readable boot status/failure reporting.

## Status
Evolving into the capability-aligned bootstrap coordinator phase.
