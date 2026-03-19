# services/servicemgr

## Purpose
Works alongside `namesvc` to provide clean distributed naming. It establishes reliable service-to-service communication mechanisms across nodes/cores.

## Responsibilities
- **Service Registration**: Tracking available service components system-wide.
- **Endpoint Lookup**: Resolving queries to physical or distributed URPC endpoints.
- **Service Migration Visibility**: Updating routing after service migration.
- **Failover Redirection**: Directing requests to healthy nodes if a component crashes.
- **Versioned Interface Discovery**: Discovering APIs with backwards compatibility limits.

## Dependencies
- **May depend on:** `lib/runtime`, `lib/ipc`, standard C library headers.
- **Must not depend on:** `subsys/*`, `ui/*`, direct kernel-private headers.

## Immediate TODOs
- Handshake with local `namesvc`.
- Propagate endpoints globally.

## Status
Stub.
