---
title: IPC Roadmap
status: Proposed
owner: Documentation Working Group
last_updated: 2026-04-25
tags:
  - docs
  - architecture
  - kernel
see_also:
  - README.md
---
# IPC Roadmap

## Current Status (v1 Baseline)
Based on current code analysis in `core/kernel/src/ipc/`:
- ✅ **Synchronous Endpoints (`ipc_endpoint.c`)**: Core `ipc_endpoint_create`, `ipc_endpoint_send`, and `ipc_endpoint_receive` APIs are implemented and considered mature.
- ✅ **Capability Delegation**: Endpoints successfully transfer capabilities across process boundaries with rights attenuation.
- ✅ **Blocking Queues**: Sender/Receiver wait queues are implemented for synchronous rendezvous.
- 🟡 **Asynchronous IPC (`async_ipc.c`)**: Basic asynchronous messaging exists. However, it relies on a fixed global request table (`g_async_requests`), which acts as a scalability bottleneck and needs transition to per-core or per-endpoint pooling.
- 🟡 **IPC Timeout (`ipc_timeout.c`)**: Timeout support for blocking IPC calls is partially implemented.
- 🔴 **Zero-Copy I/O (`zero_copy_io.c`)**: Explicit zero-copy payload transfer via capability-backed shared memory channels is deferred.

*Note: For the cross-core multikernel uRPC spine status (formerly listed here), please see the [uRPC Roadmap](../urpc/roadmap.md).*

## Near-Term Goals (Next 3-6 Months)
1. **Refactor Async IPC (Scalability)**: Replace the global async request table with per-core request pools to align with the multikernel and distributed communication architecture outlined in the new IPC ARC.
2. **Zero-Copy Shared Memory Channels**: Fully implement capability-backed shared memory rings for high-throughput VFS and Network IPC.
3. **Robust IPC Timeouts**: Ensure all blocking IPC calls can be reliably interrupted by the timer tick to prevent denial-of-service deadlocks.

## Long-Term Vision (1+ Years)
1. **Service Integration (L3 Layer)**: Transition system services (like console and namesvc) to robustly utilize both local IPC endpoints and the multikernel uRPC spine for seamless local/remote communication.
2. **Hardware-Assisted IPC**: Investigate utilizing ARM's virtualization extensions (doorbells) or specialized NPU/SmartNIC hardware queues for zero-overhead message passing.
3. **Formal Verification of IPC Paths**: Apply formal methods to prove the absence of information leaks or capability forgery in the highly optimized assembly fast-paths.