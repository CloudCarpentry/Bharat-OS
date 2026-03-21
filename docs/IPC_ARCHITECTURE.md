# Bharat-OS IPC Architecture

This document describes the design and guarantees of the Inter-Process Communication (IPC) subsystem in Bharat-OS.

## Endpoint Model
Bharat-OS utilizes capability-protected synchronous and asynchronous endpoints for secure messaging. Endpoints (`ipc_endpoint_t`) act as rendezvous points or bounded buffers for transferring messages between isolated execution contexts (processes or threads).

Every endpoint maintains:
- A message buffer.
- `senders` wait queue: Threads waiting to send a message when the buffer is full.
- `receivers` wait queue: Threads waiting to receive a message when the buffer is empty.

Access to endpoints is strongly regulated via capabilities (`send_cap` and `recv_cap`), ensuring only authorized participants can exchange data.

## Sync vs Async Semantics
### Synchronous IPC
The core `ipc_endpoint_send` and `ipc_endpoint_receive` operations are fully synchronous:
- **Send**: If an endpoint buffer is occupied, the sender enqueues itself onto the `senders` wait queue and blocks.
- **Receive**: If an endpoint buffer is empty, the receiver enqueues itself onto the `receivers` wait queue and blocks.

### Asynchronous IPC
For lockless, high-throughput applications, Bharat-OS also supports asynchronous IPC via a dedicated ring buffer (URPC) system (`lib/urpc/`), which operates independently of synchronous endpoint wait queues.

## Wait Queues and Wake-up Rules
To prevent spurious wakeups and guarantee starvation-free operations, blocked IPC operations are strictly coordinated through explicit wait queues (`wait_queue_t`).

- When a thread blocks on an IPC operation, it explicitly queues itself onto the endpoint's wait queue and calls the scheduler to transition its state to `THREAD_STATE_BLOCKED`.
- When an operation succeeds (a sender fills the buffer or a receiver consumes the buffer):
  - A successful `send` dequeues **exactly one** receiver from the `receivers` queue and explicitly wakes it using `sched_wakeup()`.
  - A successful `receive` dequeues **exactly one** sender from the `senders` queue and wakes it using `sched_wakeup()`.

This ensures FIFO fairness among waiters and prevents thundering herd problems.

## Scheduler Interaction
The IPC subsystem relies entirely on the kernel scheduler (`sched.h`) for state transitions. IPC code does **not** manipulate `THREAD_STATE_READY` directly. All state management and queueing is delegated to:
- `sched_wait_queue_enqueue`
- `sched_wait_queue_dequeue`
- `sched_wakeup`

This isolates the IPC code from internal scheduler data structures and runqueues.

## SMP / Multicore Notes
Wait queues and state transitions are architecture-independent. Endpoint state modifications (message copying, `has_msg` transitions, and wait-queue coupling) are protected with endpoint-local spinlocks in the synchronous endpoint path.

This protects correctness under multicore contention on supported architectures (x86_64, ARM64, RISC-V). For very high-core-count or NUMA-heavy systems, lock contention and queue sharding policy remain tuning targets.

## Guarantees Across Profiles / Personalities
Because IPC blocking and wake-up logic delegates entirely to the core scheduler, IPC behaves consistently across all supported CPU architectures (x86_64, ARM64, RISC-V) and subsystem personalities (Linux, Android). No architecture-specific hacks or global states (like `g_current`) are directly used within the wake-up logic.

Endpoint sizing is profile-aware at build time:
- RT-oriented profiles prioritize tighter bounded memory.
- General profiles use moderate endpoint/payload limits.
- Datacenter/NUMA-aware profiles allocate larger endpoint pools/payload ceilings.
