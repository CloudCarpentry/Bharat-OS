# Backend Telemetry Model

## Overview
The Backend Telemetry Model (`idl/monitor/accel_telemetry.idl`) tracks the performance, health, and reliability of advanced hardware backends. By accumulating counters for usage and fallbacks, `services/accelmgr/` and system administrators can measure exactly how often hardware is utilized versus falling back to software due to contention or failure.

## Key Metrics
- **`backend_selected`**: The number of successful hardware operations.
- **`fallback_count`**: The number of times software was used because hardware was absent, busy, or thermally restricted.
- **`throttled_count`**: Operations delayed or rejected due to QoS or thermal limits.
- **`faults`**: Recoverable and unrecoverable hardware faults (linked to fault domains).
- **`queue_latency`**: The time an operation spent waiting for admission.
- **`bytes_mapped`**: The volume of memory pushed to the device via `hal_dma_map`.
- **`dma_sync_failures`**: Errors resulting from cache coherency issues or lost device connectivity.

## Contract Integration
1. **Providers (`lib/runtime/`)**: Emit telemetry events during `backend_dispatch_select()` and execution (e.g., `tensor_process()`).
2. **Aggregators (`services/accelmgr/`)**: Gather these events into rolling health scores.
3. **Deciders (`services/powermgr/` or `faultmgr`)**: Trigger safe mode if faults exceed a predefined threshold.
