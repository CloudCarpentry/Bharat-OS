# services/namesvc

## Purpose
Name service / endpoint registry. Provides discovery, registration, versioning, and endpoint resolution for other services and apps. Makes URPC and endpoint IPC usable at system scale. Works in tandem with `services/servicemgr` for clean distributed naming.

## Dependencies
- **May depend on:** `lib/runtime`, `lib/ipc`, `lib/cap`, standard C library headers.
- **Must not depend on:** other `services/*` (no service-to-service compile-time dependency yet), `subsys/*`, `ui/*`.
- **Must not use:** direct kernel-private headers.

## Planned Public API
- IPC Schema: Publish Service Endpoint (Name, Version).
- IPC Schema: Resolve Service Name -> Endpoint Capability Handle.

## Immediate TODOs
- Establish the primary well-known endpoint.
- Connect to `services/init` to announce readiness.
- Implement an in-memory hash table for tracking endpoints by string name.

## Status
Stub.
