---
title: Cross-Core Thread Handoff
status: accepted
owner: Divyang Panchasara
version: 1.0
last_updated: 2024-05-24
tags: [scheduler, urpc, capabilities, multikernel]
---

# ADR: Capability-Protected Cross-Core Thread Handoff via uRPC

## Context & Problem Statement
In the multikernel architecture of Bharat-OS, each physical core runs an independent scheduler instance with local, lockless runqueues to avoid cache-line bouncing and global spinlock contention. Eventually, the system requires full distributed task migration to balance load or respond to AI Governor policies.

Full task migration is highly complex. It involves migrating register images, stack ownership, TLB coordination, addressing capabilities, and cross-core process contexts.

Before solving the hardest cross-core state-transfer problems, we need a baseline primitive that proves out the foundational interaction between the Scheduler, Tasks/Threads, Capabilities, and uRPC modules.

## Decision
We will implement **Capability-Protected Cross-Core Thread Handoff (Remote Enqueue)** as the first production-grade primitive toward task migration.

Instead of full execution context transfer, this feature handles the structured request for a core to take ownership of a "handoff-eligible" thread and enqueue it on its local runqueue.

## Scope
1. **New Thread State**: A new thread state `THREAD_STATE_REMOTE_HANDOFF_PENDING` will track a thread in transit between core runqueues. Only threads in `THREAD_STATE_READY` are eligible to be transitioned to this state.
2. **uRPC Message Contract**: A new explicit, typed uRPC message `MK_MSG_THREAD_HANDOFF_REQ` will be created (and corresponding ACK/NACK responses).
3. **Capability Validation**: The receiving core must validate that the sender holds the necessary authority (e.g. `CAP_SCHED_TARGET` or an equivalent token) before accepting the enqueue request.
4. **Target Scheduler Action**: Upon successful validation, the receiver updates thread affinity/bounds and enqueues it on its local lockless runqueue.

## Consequences
- **Positive:** Tests the full L0 uRPC fabric, validates multikernel capability passing design, and provides immediate value for static thread placement across cores. It avoids over-engineering full SMP context migration upfront.
- **Negative/Deferred:** Blocked threads, threads mid-syscall, or running threads cannot be handed off with this primitive. Full process migration and capability graph synchronization are deferred to subsequent milestones.
