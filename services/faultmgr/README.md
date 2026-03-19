# services/faultmgr

## Purpose
A critical component for the distributed OS to contain crashes and handle fault conditions system-wide.

## Responsibilities
- **Crash Containment**: Limiting the blast radius of component failure.
- **Restart Policies**: Determining if/how/when to restart failed services.
- **Service Dependency Graph**: Tracking what components rely on a failed entity.
- **Degraded Mode**: Orchestrating the system behavior when operating under a fault.
- **Node/Core Quarantine**: Isolating unreliable or consistently failing hardware blocks.
- **Fault Correlation**: Determining the root cause of cascading failures.

## Dependencies
- **May depend on:** `lib/runtime`, `lib/ipc`, standard C library headers.
- **Must not depend on:** `subsys/*`, `ui/*`, direct kernel-private headers.

## Immediate TODOs
- Connect to `coremgr` to listen for panic or health alerts.
- Start tracking critical services via `servicemgr`.

## Status
Stub.
