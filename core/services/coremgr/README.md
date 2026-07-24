# services/coremgr

## Purpose
Per-core monitor and control-plane coordinator. It acts as the nervous system of the multikernel architecture.

## Responsibilities
- **Core State Management**: Owns core online/offline operations.
- **Topology Map**: Maintains the system topology.
- **Transport Health**: Monitors the health of communication channels.
- **Message Routing**: Determines routes for cross-core messaging.
- **Service Discovery**: Coordinates cross-core service discovery.
- **Remote Execution**: Handles remote execution requests.

## Dependencies
- **May depend on:** `lib/runtime`, `lib/ipc`, standard C library headers.
- **Must not depend on:** `subsys/*`, `ui/*`, direct kernel-private headers.

## Immediate TODOs
- Establish IPC capabilities to monitor core status.
- Integrate with `telemetrymgr` for core health.

## Status
Stub.
