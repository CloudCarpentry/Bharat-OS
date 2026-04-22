# uRPC Roadmap

## Current Status (v1 Baseline)
Based on current code analysis in `kernel/src/ipc/` and `<advanced/multikernel.h>`:
- ✅ **Basic Ring Buffer API (L0 Transport)**: `urpc_init_ring`, `urpc_send`, and `urpc_receive` are implemented as lockless queues (`multikernel.c`).
- ✅ **Channel Bindings**: `mk_establish_channel` and associated multikernel init paths establish connections.
- ✅ **Wire Format**: The v1 binary message format (including canonical headers and capability wire descriptors) is defined and implemented.
- 🟡 **Capwire Translation & Validation**: The proxying of capabilities across cores via Capwire descriptors is in development. Authorization is currently stubbed (`mk_authorize_message` in `mk_dispatch.c`).
- 🟡 **Interrupt Wakeups**: Cross-core IPIs to wake sleeping threads waiting on uRPC rings are partially implemented but need architecture-specific hardening (e.g., x86 Local APIC IPIs, ARM GIC SGIs).
- 🟡 **Remote Scheduling Enqueue**: Baseline implementation of `MK_MSG_THREAD_HANDOFF_REQ` message contract, validation, and execution to test the L0 fabric.
- 🟡 **L1 Protocol Engine (`mk_proto.c`)**: Partial protocol engine now exists including policy classification, transaction registry, ACK/NACK completion, and bounded timeout state transitions. (Duplicate suppression and robust scheduling integration still pending.)
- 🔴 **BIDL Stub Generator**: The `bidlc` compiler for generating C stubs from Interface Definition Language files (`bidl-v1.md`) is currently a prototype/deferred for production.

*Note: For the local endpoint and synchronous IPC status, please see the [IPC Roadmap](../ipc/roadmap.md).*

## Near-Term Goals (Next 3-6 Months)
1. **Implement Full L1 Protocol Engine (CRITICAL)**: Build on existing policy helpers to add transaction tracking, ACK/NACK logic, and timeout handling in `mk_proto.c`.
2. **Capability-Safe Messaging**: Replace stubbed authorization (`mk_authorize_message`) in dispatch paths. Add cross-core capability validation, explicit ownership guarantees, and rollback handling.
3. **Interrupt Mitigation (NAPI style)**: Implement adaptive polling for uRPC rings. When a core receives an IPI, it should poll the ring for a short window to process subsequent messages before going back to sleep, reducing IPI overhead.
4. **Stabilize Capwire Proxying**: Ensure that transferring a capability via uRPC correctly creates a proxy object on the receiving core and that revocation flows backward across the multikernel interconnect.

## Long-Term Vision (1+ Years)
... (rest unchanged)
