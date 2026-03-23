# uRPC Roadmap

## Current Status (v1 Baseline)
Based on current code analysis in `kernel/src/urpc/` and `<advanced/multikernel.h>`:
- ✅ **Basic Ring Buffer API**: `urpc_init_ring`, `urpc_send`, and `urpc_receive` are implemented as lockless queues.
- ✅ **Channel Bindings**: `urpc_channel_bind`, `accept`, and `close` are present for establishing connections.
- ✅ **Wire Format**: The v1 binary message format (including canonical headers and capability wire descriptors) is defined and implemented.
- 🟡 **Capwire Translation**: The proxying of capabilities across cores via Capwire descriptors is in development; the basic serialization works, but complex derivation chains across nodes require more robust tracking.
- 🟡 **Interrupt Wakeups**: Cross-core IPIs to wake sleeping threads waiting on uRPC rings are partially implemented but need architecture-specific hardening (e.g., x86 Local APIC IPIs, ARM GIC SGIs).
- 🔴 **BIDL Stub Generator**: The `bidlc` compiler for generating C stubs from Interface Definition Language files is currently a prototype/deferred for production.

## Near-Term Goals (Next 3-6 Months)
1. **Stabilize Capwire Proxying**: Ensure that transferring a capability via uRPC correctly creates a proxy object on the receiving core and that revocation flows backward across the multikernel interconnect.
2. **Interrupt Mitigation (NAPI style)**: Implement adaptive polling for uRPC rings. When a core receives an IPI, it should poll the ring for a short window to process subsequent messages before going back to sleep, reducing IPI overhead.
3. **Formalize BIDL**: Complete the first version of the Interface Definition Language and the C stub generator to replace manual message packing in user-space services.

## Long-Term Vision (1+ Years)
1. **Distributed Multikernel**: Extend the uRPC transport layer beyond shared memory (PCIe/CXL) to support RDMA over Ethernet, allowing Bharat-OS to span multiple physical machines transparently.
2. **Zero-Copy Network Stack**: Tightly integrate the uRPC out-of-line (OOL) descriptor model with the network driver's DMA rings to achieve true zero-copy packet processing from the NIC directly to the user-space application.
3. **Hardware Ring Offload**: Investigate offloading the uRPC ring buffer management to SmartNICs or NPUs to free CPU cycles entirely from message passing overhead.