# uRPC (User-level Remote Procedure Call)

## Overview

The `urpc` cluster documents the foundational communication layer of the Bharat-OS multikernel architecture. While synchronous IPC endpoints (see [../ipc/README.md](../ipc/README.md)) handle local, intra-core communication, **uRPC** (User-level Remote Procedure Call) provides asynchronous, lockless message passing for cross-core and cross-node coordination.

For the detailed architectural roadmap and multikernel IPC specifications, see [IPC + uRPC + Multikernel Communication Architecture](../../core/ipc-urpc-multikernel-arc.md).

## Why uRPC over POSIX IPC?

In traditional SMP (Symmetric Multiprocessing) operating systems (like Linux), cores coordinate by acquiring global spinlocks to modify shared data structures (e.g., page tables, file descriptors). On modern hardware with 64+ cores, this cache-coherence traffic (cache-line bouncing) becomes a massive bottleneck.

Bharat-OS adopts the **Multikernel** model (inspired by Barrelfish):
1.  **No Shared State:** Each core runs an independent instance of the kernel. Data structures are replicated, not shared.
2.  **Explicit Messaging:** Cores coordinate *exclusively* via explicit messages sent over the hardware interconnect (PCIe, QPI, HyperTransport, or a Network-on-Chip).
3.  **Lockless:** Cores never acquire spinlocks that belong to other cores.

uRPC is the protocol that implements this explicit, lockless message passing. It operates over shared memory ring buffers, completely bypassing the traditional POSIX IPC abstractions (pipes, message queues) which are too slow and heavyweight for core-to-core kernel synchronization.

## Cross-Core Transport (uRPC / Multikernel)
**Maturity:**
- **L0 Transport:** Baseline / Implemented
- **L1 Protocol / Delivery Engine:** Scaffold / Missing
- **Distributed Capability Safe Messaging:** Partial / Incomplete

For cross-core multikernel messaging, the OS uses the **uRPC transport**. This relies on asynchronous, cache-friendly SPSC ring buffers established between cores (located in `core/kernel/src/ipc/multikernel.c` and `core/kernel/include/advanced/multikernel.h`).

While the fundamental L0 uRPC transport (ring buffers, acquire/release ordering, basic delivery hooks) is in place, the **L1 Protocol Engine** (responsible for ACK/NACK semantics, retry policies, transaction lifecycles, and timeouts) is currently stubbed out or scaffolded. Additionally, explicit ownership and cross-core capability validations are still incomplete.

## Multikernel URPC API Baseline

Defined in `<advanced/multikernel.h>`:

- `int urpc_init_ring(urpc_ring_t* ring, urpc_msg_t* buffer_ptr, uint32_t ring_size)`
  - Initializes a URPC ring on top of a backing message buffer.
- `int urpc_send(urpc_ring_t* ring, const urpc_msg_t* msg)`
  - Lockless send to a URPC ring.
- `int urpc_receive(urpc_ring_t* ring, urpc_msg_t* out_msg)`
  - Lockless receive from a URPC ring. Does not block; returns `URPC_ERR_EMPTY` if no message is pending.

## Related Documents
- [Call Model](call-model.md) - Synchronous call/reply, asynchronous send, and one-way datagrams.
- [Endpoint Design](endpoint-design.md) - How capabilities relate to uRPC channels.
- [Marshalling (Wire Format)](marshalling.md) - The IDL, serialization, and Capwire descriptors (formerly `msg-wire-format-v1.md`).
- [Kernel Path](kernel-path.md) - The fast path through the kernel and register ABIs.
- [User-space Stubs](userspace-stub.md) - How IDL stubs are generated and linked.
- [Roadmap](roadmap.md) - Current status and future goals for uRPC.