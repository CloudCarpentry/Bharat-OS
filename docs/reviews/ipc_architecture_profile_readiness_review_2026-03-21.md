# IPC Architecture Critical Review (March 21, 2026)

This review evaluates `docs/IPC_ARCHITECTURE.md` against the current implementation and assesses readiness across:

- automotive (RT + mixed criticality),
- desktop,
- edge/embedded,
- ARM, RISC-V, x86_64,
- datacenter,
- network appliance,
- multikernel deployments.

## 1) What already aligns well

1. **Capability-gated endpoint access** is implemented for send/receive operations (`cap_table_lookup` with endpoint type + rights checks).
2. **FIFO wakeup behavior** exists through explicit sender/receiver wait queues and single waiter dequeue semantics.
3. **Bounded endpoint message model** (single-slot endpoint payload) provides deterministic memory behavior useful for RT/edge.
4. **URPC ring uses explicit atomic memory ordering**, which is a strong base for multicore/multikernel messaging.
5. **Async timeout heap model** is deterministic and avoids linear scans for timeout dispatch.

## 2) Critical gaps found

### A. Doc/implementation mismatch around SMP safety

`docs/IPC_ARCHITECTURE.md` states SMP protection is still a note for future locking; the code previously manipulated endpoint state without endpoint-local locking in hot paths (`ep->has_msg`, payload copy, queue transitions).

**Risk:** race conditions and inconsistent wakeups under multicore contention.

### B. Profile scaling was fixed-size and not profile-aware

Endpoint pool and payload capacity were hard-coded, which does not scale from RT/automotive control nodes (small deterministic buffers) to datacenter/network profiles (higher concurrency and larger control messages).

### C. User-space IPC facade is still stubbed

`lib/ipc/src/ipc.c` is entirely unimplemented (`-1` stubs), so user-space profile behavior currently relies on lower-level/kernel-only paths and tests.

### D. Distributed/multikernel IPC path still partial

Cross-core channel establishment and ring transport exist, but delivery integration remains partial in `multikernel.c` (drain/dispatch path is not fully realized end-to-end in this snapshot).

## 3) Immediate implementation tasks completed in this change

## Task 1 — Add endpoint-level SMP synchronization (implemented)

Implemented endpoint-local spinlocks to serialize:

- `has_msg` state transitions,
- payload/capability metadata writes and reads,
- sender/receiver wait-queue coordination.

This closes the most immediate multicore correctness gap for synchronous endpoint IPC.

## Task 2 — Make endpoint sizing profile-aware (implemented)

Introduced profile-driven compile-time IPC capacity policy:

- **RT profile:** smaller payload and endpoint set for deterministic footprint.
- **General profile:** moderate defaults.
- **Datacenter/NUMA-aware profile:** larger pool and payload ceiling.

This gives a concrete path to tune IPC without rewriting IPC core logic for each deployment class.

## 4) Recommended next implementation tasks (not yet done)

1. **Implement `lib/ipc` syscall bridge and URPC fast-path selection**
   - wire `bharat_ipc_send/recv/call` to kernel UAPI,
   - support profile-aware path selection (sync endpoint vs URPC ring),
   - add ABI-stable error mapping.

2. **Add endpoint observability counters**
   - per-endpoint drops/timeouts/block counts,
   - profile-exported telemetry hooks for datacenter/network and automotive diagnostics.

3. **Introduce stronger timeout semantics**
   - absolute deadlines (not only relative ticks),
   - profile policy for timeout admission and budget overruns (especially hard RT).

4. **Complete multikernel delivery integration**
   - finalize channel drain-to-dispatch flow,
   - add remote completion/ack semantics,
   - add congestion/backpressure policy per profile.

5. **Harden mixed-criticality controls**
   - priority-aware queueing per endpoint class,
   - optional partitioned endpoint pools for safety-critical vs best-effort traffic.

## 5) Profile/architecture readiness snapshot

- **Automotive RT / mixed criticality:** improved by deterministic bounded sync IPC + new endpoint locking; still missing stronger admission and telemetry/audit hooks.
- **Desktop:** functionally serviceable in kernel tests; missing full userspace IPC wrapper implementation.
- **Edge embedded:** good footprint direction with profile-sized endpoint settings.
- **Datacenter/network appliance:** better scalability knobs now exist; still need deep observability and distributed ack/backpressure hardening.
- **ARM / RISC-V / x86_64:** lock/wait-queue behavior is architecture-neutral; URPC already uses arch-portable atomics.

## 6) Conclusion

The current IPC architecture is directionally sound, but this change addresses two concrete production-readiness blockers now:

1. multicore race hardening in endpoint IPC;
2. profile-scalable endpoint capacity policy.

The next most important closure is completing the userspace IPC wrapper and multikernel delivery completion semantics.
