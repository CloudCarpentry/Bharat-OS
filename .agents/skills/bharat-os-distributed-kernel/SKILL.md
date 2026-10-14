---
name: bharat-os-distributed-kernel
description: Apply to per-core ownership, scheduler migration, TLB shootdown, distributed VM, IPC/uRPC, monitor messages, remote lifecycle, or cross-core transactions.
---

# Bharat-OS Distributed Kernel Skill

## Protocol Invariants

1. **Strict Ownership**: Every mutable kernel object has exactly one owner core at a given instant.
2. **Wire Boundary Discipline**:
   - Remote participants receive fixed-width, versioned, by-value messages.
   - **Never pass raw pointers, stack addresses, heap addresses, function pointers, or architecture-dependent `long`/`size_t` fields.**
   - All wire structs must have static compile-time size and alignment assertions (`static_assert`).
3. **Generations and Request IDs**:
   - Every transaction carries an explicit request ID and generation counter to defeat ABA and stale-handle confusion.
4. **Monotonic Timing & Bounded Waits**:
   - Deadlines MUST be calculated using monotonic HAL time (`bh_ktime_now()`).
   - **Strictly prohibit spin-count loops** (e.g. `for (int i=0; i<timeout_loops; i++) arch_cpu_relax();`). All remote waits must have bounded monotonic timeouts.
5. **No Locks During Remote Waits**:
   - Never hold a scheduler runqueue lock, address-space lock, capability table lock, or object mutex while waiting for a remote core to ACK or reply.
6. **Bounded Queues & Backpressure**:
   - Queues must have compile-time fixed capacities. Queue-full returns `BH_STATUS_ERR_BUSY` or explicit backpressure.
7. **Idempotence & Replay Safety**:
   - Retried requests must be safe to re-execute without corrupting state.
   - Retries must target only unacknowledged or missing participant cores.
8. **Failure & Rollback**:
   - Every multi-step protocol must define compensation/rollback for partial failures.
   - If consistent rollback is impossible, poison or quarantine the object and fail closed.
9. **Destruction Lifecycle**:
   - Teardown requires an explicit lifecycle state (e.g. `BH_OBJ_STATE_DYING`).
   - New mutations must be rejected immediately upon entering the dying state.

## Implementation Checklist

- [ ] Wire structures defined with `uint32_t`, `uint64_t`, explicit padding, and static assertions.
- [ ] Core-local transaction table allocated with bounded slots.
- [ ] Monotonic deadline set with `bh_ktime_now() + timeout_ticks`.
- [ ] Locks released before entering wait loop, reacquired and revalidated on return.
- [ ] Retries filtered by missing core bitmask.
- [ ] Rollback logic verified on injected remote failure or timeout.
- [ ] SMP stress tests implemented covering concurrent transactions, timeouts, and queue saturation.
