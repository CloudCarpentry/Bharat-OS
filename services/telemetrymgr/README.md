# services/telemetrymgr

## Purpose
System-wide metrics and monitoring service. Needed early to prevent multikernel development from becoming guesswork.

## Responsibilities
- **Core-Level Metrics**: Tracking CPU utilization, instructions per cycle, etc.
- **Transport Latency**: Monitoring delay between nodes/cores.
- **Queue Depth**: Observing congestion points in network, storage, or accelerators.
- **Page-Fault Rates**: Helping `memmgr` determine paging behavior effectiveness.
- **IRQ Storms**: Identifying hardware or driver problems early.
- **Accelerator Utilization**: Knowing when offload engines are saturated.
- **Migration Events**: Recording times tasks or memory shift contexts.
- **Power/Thermal Signals**: Relaying environmental boundaries to `schedmgr` and `coremgr`.

## Dependencies
- **May depend on:** `lib/runtime`, `lib/ipc`, standard C library headers.
- **Must not depend on:** `subsys/*`, `ui/*`, direct kernel-private headers.

## Immediate TODOs
- Aggregate metrics from the kernel dispatcher, `memmgr`, and `coremgr`.
- Set up a simple shared-memory ring buffer for low-overhead logging.

## Status
Stub.
