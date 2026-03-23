---
title: Bharat-OS Profile-Driven Multi-Kernel Architecture
status: proposed
owner: kernel-architecture
reviewers: []
version: 0.1
last_updated: 2026-03-23
tags:
  - kernel
  - architecture
  - ipc
  - urpc
  - capability
  - multi-kernel
  - distributed
---

# ARC: Bharat-OS Profile-Driven Multi-Kernel Architecture

## 1. Status

Proposed

## 2. Decision Summary

Bharat-OS will be positioned as a **profile-driven, capability-governed, communication-centric kernel platform** rather than as a single monolithic kernel with optional features.

The platform will support:

* **RT profile** for bounded and analyzable execution
* **GP profile** for richer services and throughput-oriented operation
* **MIX profile** for explicit RT/GP coexistence
* future **same-machine multi-kernel partitions**
* future **distributed-kernel extensions**

The architectural backbone will be:

* **Capabilities** as the authority and isolation model
* **IPC** as the broad communication plane
* **uRPC** as a structured request/reply subset within that IPC plane
* **AI Governor** as an optional, selectable governance layer that never bypasses capability or profile rules

## 3. Context

The current direction implies:

* boot/profile separation is becoming important
* scheduler, MM, TLB, IRQ, and domain separation must align with profile semantics
* subsystem and service interactions should move away from ad hoc reach-through coupling
* capability control is central to safe hybrid and future distributed operation
* communication must be modeled explicitly instead of treating all traffic as one mechanism

The main architectural correction is this:

* **IPC and uRPC must not be collapsed into the same concept**
* **IPC is the umbrella communication family**
* **uRPC is one structured, typed, capability-scoped interaction style inside IPC**

## 4. Core Architectural Thesis

Bharat-OS is a **single codebase with multiple policy realizations**.

RT, GP, and MIX are not separate products. They are different policy profiles over a shared kernel core.

That shared core should converge around:

* boot and profile framework
* task/thread model
* protection domain and address-space model
* capability model
* object model
* IPC core
* uRPC subsystem
* scheduler framework
* timer and IRQ framework
* PMM/VMM/TLB primitives
* observability and audit hooks
* optional governance interfaces

## 5. Communication Architecture

### 5.1 IPC as the umbrella plane

IPC is the full communication plane and should include:

* synchronous and asynchronous messaging
* notifications and events
* queues and mailboxes
* wait/wake objects
* shared-memory channels
* stream and bulk-data channels
* cross-core and cross-domain transport
* future cross-partition and cross-node transport

### 5.2 uRPC as a structured subset

uRPC is not the whole communication model. It is a structured subset suited for:

* service invocation
* namespace and object operations
* capability queries
* scheduler and policy control
* subsystem and management calls
* bounded command paths

Rule:

* **all uRPC is IPC**
* **not all IPC is uRPC**

### 5.3 Traffic-type mapping

Communication should be chosen by traffic type.

| Traffic type                 | Preferred mechanism         |
| ---------------------------- | --------------------------- |
| control-plane call           | uRPC                        |
| service/object operation     | uRPC                        |
| event/notification           | lightweight IPC             |
| telemetry stream             | IPC stream or queue         |
| bulk/high-rate data          | shared-memory IPC           |
| deferred work handoff        | bounded IPC queue           |
| cross-partition service call | uRPC over transport adapter |
| future remote service call   | uRPC over remote transport  |

## 6. Capability Model

Capabilities should become the **kernel-wide authority model**, not a narrow security feature.

They should govern:

* tasks and threads
* address spaces and protection domains
* memory objects
* IPC endpoints and channels
* shared-memory export/import/map rights
* timers and IRQ/event sources
* devices and DMA windows
* service registration and lookup
* scheduler control rights
* domain bridge rights
* future remote endpoint and remote object rights

### 6.1 Communication capability split

IPC-related rights should include:

* create endpoint
* bind/connect endpoint
* send/receive
* notify/wait
* create queue/channel
* export/import/map shared memory
* use bridge/channel between domains

uRPC-related rights should include:

* invoke service
* reply on endpoint
* register service
* restrict visible methods
* perform privileged control calls
* bridge calls across domains

## 7. Profile Model

### 7.1 RT profile

RT should use only bounded, analyzable communication subsets.

Rules:

* pre-created channels/endpoints in critical paths
* fixed queue sizing
* bounded allocation behavior
* deterministic event delivery
* RT-safe subset of uRPC only
* bulk and high-rate paths should prefer shared-memory IPC or fixed channels
* no unbounded service discovery in critical paths

### 7.2 GP profile

GP can use the full communication stack.

Rules:

* full uRPC service composition
* dynamic service registration where appropriate
* async IPC queues and streams
* broader scheduler classes
* richer namespace and service interaction
* heavier tracing and background activity allowed

### 7.3 MIX profile

MIX is the key architecture profile.

Rules:

* RT and GP policy domains coexist explicitly
* CPU, IRQ, memory, and service placement are partition-aware
* RT↔GP control path uses uRPC
* RT↔GP event path uses lightweight IPC
* RT↔GP bulk/telemetry path uses shared-memory IPC or rings
* deferred work crosses domains only through bounded queues
* capabilities govern every bridge and shared object

## 8. AI Governor Position

AI Governor should be optional and selectable.

It must not sit inside the correctness core and must not bypass capability or profile rules.

Recommended split:

* **uRPC** for control and policy operations
* **IPC streams/queues/shared memory** for telemetry, trace, anomaly, and metrics ingestion

That keeps AI governance useful without making the kernel behavior opaque or unsafe.

## 9. Multi-Kernel and Distributed Evolution

### 9.1 Near-term interpretation

Near term, multi-kernel means:

* one kernel image
* multiple policy domains
* explicit RT/GP partitioning
* bridge communication through IPC/uRPC abstractions

### 9.2 Same-machine partitioned multi-kernel

Later, Bharat-OS can evolve to:

* RT partition and GP partition on the same machine
* shared memory + mailbox/IPI transport adapters
* cross-partition endpoint routing
* capability-mediated bridge exposure

### 9.3 Distributed-kernel direction

Distributed extension should reuse the same model:

* node-aware transport adapters
* remote endpoint addressing
* remote capability envelopes
* service discovery
* fault-aware message delivery

Non-goal for early phases:

* full transparent distributed shared memory
* fake locality that hides latency/failure

## 10. Code-Structure Guidance

The architecture should drive code organization around these areas:

### 10.1 Boot / profile / mode

Boot should formally select:

* profile
* scheduler set
* memory policy
* IRQ policy
* service topology
* endpoint registry
* RT/GP partition layout
* optional AI Governor enablement
* distributed feature flags

### 10.2 IPC/uRPC subsystem

This should become an explicit subsystem with:

* endpoint objects
* message model
* invoke/reply semantics
* async notification model
* shared-memory fast path
* backpressure rules
* timeout/cancel semantics
* cross-core/cross-domain adapters
* future remote transport shim

### 10.3 Scheduler alignment

Scheduler should become domain-aware and communication-aware:

* RT class
* GP class
* partition/group scheduling
* analyzable wakeup/dispatch semantics
* capability-protected scheduler control

### 10.4 Memory alignment

MM/TLB/VMM/PMM work should align with domain separation:

* RT-safe pools
* GP pageable/reclaimable pools
* shared-memory objects for cross-domain exchange
* capability-controlled mapping
* explicit shootdown/coordination paths

## 11. Risks and Architectural Traps

* Treating IPC and uRPC as the same thing will muddy the design.
* Forcing all traffic through RPC semantics will hurt RT behavior.
* Letting GP-style dynamism leak into RT critical paths will break analyzability.
* Using capabilities only for userspace-facing APIs will leave internal boundaries weak.
* Hiding remote behavior behind fake-local assumptions will damage future distributed correctness.
* Making AI Governor mandatory or overly privileged will undermine trust and debuggability.

## 12. Recommended Phased Plan

### Phase 0 — Formalize the architecture

Deliver:

* communication taxonomy
* capability taxonomy
* object taxonomy
* profile definitions
* traffic-type mapping
* near-term vs long-term multi-kernel definitions

### Phase 1 — Harden IPC core

Deliver:

* endpoint abstractions
* queue/mailbox semantics
* notification/event primitives
* wait/wake integration
* shared-memory channel primitives
* tracing/stats/audit hooks

### Phase 2 — Harden uRPC subsystem

Deliver:

* typed interface model
* invoke/reply semantics
* timeout/cancel rules
* capability checks
* RT-safe subset and GP-full subset
* service registration/dispatch design

### Phase 3 — Make profile selection real

Deliver:

* boot-time profile selection
* profile manifests/config
* per-profile scheduler/memory/IRQ policy binding
* allowed transport matrix per profile
* AI Governor enable/disable policy

### Phase 4 — Build MIX bridge architecture

Deliver:

* RT↔GP control via uRPC
* RT↔GP event path via IPC
* RT↔GP shared-memory bulk path
* bounded deferred work queues
* bridge capabilities and auditability

### Phase 5 — Align scheduler, MM, IRQ, timer, and device ownership

Deliver:

* scheduler classes and domain-aware dispatch
* RT-safe memory pools
* capability-controlled shared memory mapping
* IRQ/device ownership rules
* profile compatibility matrix for subsystems

### Phase 6 — Same-machine multi-kernel prototype

Deliver:

* partition-local service model
* cross-partition transport adapters
* shared memory + mailbox/IPI prototype
* endpoint routing across partitions

### Phase 7 — Distributed-kernel foundation

Deliver:

* remote endpoint routing
* remote capability envelope format
* service discovery model
* fault-aware messaging semantics
* node transport abstraction

## 13. Immediate Priorities

1. Freeze the conceptual model: IPC umbrella, uRPC subset, capability-governed everything.
2. Turn profile selection into a formal boot/runtime contract.
3. Make IPC/uRPC an explicit subsystem instead of scattered helpers.
4. Define capability types and rights for communication, memory, IRQ, devices, and scheduler control.
5. Implement MIX bridges deliberately instead of allowing ad hoc RT↔GP coupling.

## 14. Final Position

Bharat-OS should be described as a **profile-driven, capability-governed, communication-centric kernel platform**.

Its communication architecture is built around a general **IPC plane** and a structured **uRPC subsystem**. RT, GP, and MIX profiles choose bounded subsets and policies over that common foundation. Same-machine multi-kernel and future distributed-kernel evolution should extend the same object, capability, IPC, and uRPC model instead of introducing a second architectural story.
