# Capabilities Roadmap

## Current Status (v1 Baseline)
Based on current code analysis in `kernel/src/cap/` and `kernel/src/ipc/`:
- ✅ **Basic Capability Table (`capability_table_t`)**: Implemented.
- ✅ **Capability Entries**: Implemented types (`SEND`, `RECEIVE`, `MAP`, `UNMAP`, `SCHEDULE`, `DELEGATE`).
- ✅ **Capability Transfer Policy**: Basic `cap_can_transfer` and `cap_transfer_rights_valid` exist to enforce rights attenuation.
- ✅ **Capability Delegation**: Implemented via IPC Endpoint synchronous send/receive (`ipc_endpoint_send`, `ipc_endpoint_receive`).
- 🟡 **CNode Hierarchy**: Currently, tables are somewhat flat per process (`process_create`). The full CNode tree graph for complex derivations is stabilizing.
- ✅ **Revocation**: Recursive revocation paths are semantically verified for deep derivation chains within and across process capability tables. Generation invalidation and bounds iteration ensure safety.
- 🔴 **Untyped Retyping**: Explicit memory provisioning for kernel objects from user space is currently deferred; the kernel still performs some implicit allocations during early boot.

- 🟡 **Remote Scheduling Authority**: Defining and testing minimum authority patterns (e.g., `SCHEDULE` on thread + `TARGET_CORE` authorization) for safely requesting remote cross-core thread handoff over uRPC.

## Near-Term Goals (Next 3-6 Months)
1. **Full CNode Tree Implementation**: Finalize the directed graph structure for capability spaces to allow arbitrary nesting and delegation depths, replacing optimistic iteration models with scalable RCUs or epochs.
2. **Strict Revocation Verification**: (Mostly achieved) Ensure that revoking a parent capability deterministically and safely destroys all derived children across all processes. Cross-core distributed transport logic requires completion.
3. **Untyped Memory Retyping**: Transition all kernel object allocations (TCBs, Endpoints, Page Tables) to require explicit user-space `Untyped` capability derivation.
4. **Distributed Authorization Boundaries**: Integrate explicit capability validations into uRPC dispatch paths to replace placeholder authorization checks.

## Long-Term Vision (1+ Years)
1. **Capability Wire Format (Capwire)**: Formalize the serialization of capabilities for transfer over the multikernel URPC ring buffer to remote cores. This is essential for the "Multikernel Spine".
2. **Sealed Capabilities**: Implement the `SEAL` and `UNSEAL` capability types for opaque state passing between mutually untrusting user-space services.
3. **Hardware Capability Support**: Research integration with hardware capability architectures like CHERI (Capability Hardware Enhanced RISC Instructions) for spatial and temporal memory safety at the instruction level.