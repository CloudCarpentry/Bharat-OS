# IPC Roadmap

## Current Status (v1 Baseline)
Based on current code analysis in `kernel/src/ipc/`:
- ✅ **Synchronous Endpoints (`ipc_endpoint.c`)**: Core `ipc_endpoint_create`, `ipc_endpoint_send`, and `ipc_endpoint_receive` APIs are implemented.
- ✅ **Capability Delegation**: Endpoints successfully transfer capabilities across process boundaries with rights attenuation.
- ✅ **Blocking Queues**: Sender/Receiver wait queues are implemented for synchronous rendezvous.
- 🟡 **Asynchronous IPC (`async_ipc.c`)**: Basic asynchronous messaging exists but is being refined for the URPC ring transition.
- 🟡 **IPC Timeout (`ipc_timeout.c`)**: Timeout support for blocking IPC calls is partially implemented.
- 🔴 **Zero-Copy I/O (`zero_copy_io.c`)**: Explicit zero-copy payload transfer via capability-backed shared memory channels is deferred.
- 🔴 **Multikernel URPC (`multikernel.c`, `mk_dispatch.c`)**: The cross-core Barrelfish-style multikernel messaging spine is in early development.

## Near-Term Goals (Next 3-6 Months)
1. **Zero-Copy Shared Memory Channels**: Fully implement capability-backed shared memory rings for high-throughput VFS and Network IPC.
2. **Fast-Path Assembly Stubs**: Optimize the `ipc_endpoint_send/receive` paths with architecture-specific assembly to guarantee <500 cycle context switches on x86_64 and ARM64.
3. **Robust IPC Timeouts**: Ensure all blocking IPC calls can be reliably interrupted by the timer tick to prevent denial-of-service deadlocks.

## Long-Term Vision (1+ Years)
1. **Multikernel URPC Spine**: Transition all cross-core kernel coordination (TLB shootdowns, capability revocation, process migration) to use the lockless URPC ring buffer (`mk_proto.c`).
2. **Hardware-Assisted IPC**: Investigate utilizing ARM's virtualization extensions (doorbells) or specialized NPU/SmartNIC hardware queues for zero-overhead message passing.
3. **Formal Verification of IPC Paths**: Apply formal methods to prove the absence of information leaks or capability forgery in the highly optimized assembly fast-paths.