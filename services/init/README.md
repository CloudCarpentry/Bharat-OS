# services/init

## Purpose
The first user-space root supervisor. It launches service graphs, enforces startup order, handles restart policies, and chooses which services to bring up by profile. This replaces ad hoc startup logic and manages the tiered functionality and multiple deployment classes.

## Dependencies
- **May depend on:** `lib/runtime`, `lib/ipc`, `lib/cap`, standard C library headers.
- **Must not depend on:** other `services/*` (no service-to-service compile-time dependency yet), `subsys/*`, `ui/*`.
- **Must not use:** direct kernel-private headers.

## Planned Public API
- Subsystem init hooks.
- Process spawn logic (fork/exec equivalents, or native spawn capabilities).
- Service registration and lifecycle control endpoints.

## Immediate TODOs
- Intake the root bootstrap capability.
- Load the startup service manifest based on the hardware profile.
- Spawn `namesvc` and `capmgr`.
- Delegate bootstrap capabilities to child services.

## Status
Stub.
