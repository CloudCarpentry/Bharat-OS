---
name: bharat-os-distributed-kernel
description: Apply to per-core ownership, scheduler migration, TLB shootdown, distributed VM, IPC/uRPC, monitor messages, remote lifecycle, or cross-core transactions.
---

# Bharat-OS Distributed Kernel Skill

## Protocol invariants

- Mutable state has one owner core at a time.
- Remote participants receive by-value fixed-width messages; no pointer crosses the boundary.
- Requests carry version, operation, origin/destination, request slot/ID, and generation where stale reuse is possible.
- Queues are bounded and queue-full behavior is explicit.
- Deadlines use monotonic HAL ticks/time and are converted safely.
- Retries are bounded and target only missing participants.
- Receivers are idempotent and replay-safe.
- Completion is explicit ACK/NACK with result status.
- Local publish follows required remote ACK.
- No object/space/runqueue lock is held during remote wait/poll.
- Partial failure has compensation; irrecoverable inconsistency poisons/quarantines the object.
- Destruction moves to a terminal lifecycle phase and rejects new mutations.

## Procedure

1. Draw the ownership and message flow.
2. Define wire structures and add size/offset static assertions.
3. Define transaction allocation, generation rollover, timeout, retry, replay cache, and completion cleanup.
4. Define lock release/reacquisition and revalidation points.
5. Define rollback for every mutation stage.
6. Add diagnostics that capture missing core mask, request/generation, operation, attempts, and last status without exposing pointers.
7. Test queue saturation, duplicate delivery, delayed ACK, stale generation, timeout, partial ACK, concurrent mutation, rollback, and destroy races.
8. Run SMP QEMU validation on every architecture that supports SMP; report unsupported SMP targets explicitly.
