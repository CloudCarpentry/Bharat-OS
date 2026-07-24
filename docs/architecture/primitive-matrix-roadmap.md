---
title: Bharat-OS Primitive Matrix and Execution Roadmap
status: Proposed
owner: Documentation Working Group
last_updated: 2026-04-25
tags:
  - docs
  - architecture
see_also:
  - README.md
---
# Bharat-OS Primitive Matrix and Execution Roadmap

## Governing rule

Keep the kernel focused on deterministic mechanisms, push policy into services, keep richer composed behavior in stacks, and use personalities for Linux/Android ABI translation.

## Primitive matrix

| Primitive / Contract | Where it belongs | Why it matters for Linux translation | Why it matters for Android translation | Priority |
| --- | --- | --- | --- | --- |
| **Per-core ownership model** | **Kernel** | Lets Linux personality map threads/processes onto a real multikernel-safe substrate instead of fake shared-global state. | Needed for binder-like, service-heavy Android execution without collapsing into global kernel bottlenecks. | **P0** |
| **Message-only cross-core action contract** | **Kernel + interface/uapi/lib** | Makes remote wakeup, remote lookup, TLB/IPI-style coordination explicit. | Supports Android service/process model with strong isolation and scalable SMP. | **P0** |
| **Bounded invalidation protocol** (request id, ack, timeout, fail report) | **Kernel + HAL** | Required for correct `mmap`/`fork`/process VM semantics under Linux compatibility. | Critical for app/service churn, graphics buffers, shared mappings. | **P0** |
| **Per-core PMM caches / magazines** | **Kernel** | Gives stable allocator behavior beneath Linux-facing memory APIs. | Reduces latency spikes on mobile/embedded profiles. | **P0** |
| **Unified VM authority path** (`fault -> aspace -> object/region -> HAL`) | **Kernel** | Essential for correct POSIX/Linux VM semantics and future file-backed/shared memory behavior. | Needed for ashmem-like / dmabuf-like / graphics-oriented Android memory flows. | **P0** |
| **Capability lifecycle** (create, transfer, narrow, revoke, stale reject) | **Kernel + services + uapi** | Linux personality can expose familiar handles/fds on top, but real authority stays capability-grounded. | Android needs secure service mediation, not just UID-based checks. | **P0** |
| **Strict IPC authorization at service boundaries** | **Services + libipc/uapi** | Prevents Linux compatibility layer from bypassing the real security model. | Mandatory for system_server-style service safety. | **P0** |
| **Subsystem/profile registry** | **Kernel for mechanism, service for policy** | Allows Linux personality to boot only what that profile needs. | Android variants can boot phone/tablet/TV/automotive subsets cleanly. | **P1** |
| **Memory class tags** (`NORMAL`, `DMA`, `RT`, `SECURE`, `PACKET`, `LOWPOWER`, `PERSISTENT`) | **Kernel API + services consume** | Lets Linux allocators map down to meaningful Bharat-OS semantics. | Great for graphics, camera, audio, and low-power mobile paths. | **P1** |
| **Timer / deadline / latency-class primitives** | **Kernel** | Linux RT and time-sensitive workloads need a clean deadline-aware substrate. | Important for audio, camera, UI jank reduction, automotive Android. | **P1** |
| **Fault domains + restart policy metadata** | **Kernel tags, service decides policy** | Linux processes/services can be contained without whole-system panic. | High value for Android automotive, medical, appliance, kiosk profiles. | **P1** |
| **Service supervisor / lifecycle runtime** | **Services/core** | Lets Linux-facing services run as managed OS components. | Very important for Android-style long-lived system daemons. | **P1** |
| **Topology-aware placement hints** (cluster/cache/perf-efficiency class) | **Kernel + HAL/platform** | Helps Linux personality schedule smarter across heterogeneous CPUs. | Important for big.LITTLE Android-class hardware. | **P1** |
| **Power / thermal event hooks** | **Kernel primitive, policy in service** | Linux cpufreq/thermal-like behavior can be translated without hardcoding policy into kernel. | Critical for phones, TV, edge, and automotive thermal budgets. | **P1** |
| **Device class registry** | **Services/devmgr + drivers** | Linux personality can map a `/dev`-style surface over generic device classes. | Android HAL-like adaptation becomes cleaner. | **P1** |
| **DMA / IOMMU lifecycle contract** | **Kernel + HAL + drivers** | Needed for safe device mappings under Linux driver compatibility work. | Required for graphics/camera/accelerator security. | **P1** |
| **Telemetry / health / audit event export** | **Kernel exports, services aggregate** | Helps debug personality correctness and runtime conformance. | Needed for production Android-class observability and safety cases. | **P1** |
| **Translation-grade object model** (handle, event, wait, shared memory, queue) | **Kernel + uapi + lib** | Real substrate for mapping POSIX/Linux objects cleanly. | Also underpins binder-adjacent and surfaceflinger-adjacent designs. | **P1** |
| **Netif + routing substrate** | **Stacks/net + core/services/network** | Linux networking compatibility needs a real network stack surface. | Android networking stack depends on this being real, not stubbed. | **P2** |
| **Persistent event / crash log contract** | **Stacks/storage + core/services/system** | Helps post-mortem debug of Linux personality/runtime faults. | Important for Android recovery and field diagnostics. | **P2** |
| **Secure update / rollback lifecycle** | **Services/system + core/stacks/storage** | Supports distro-like upgrades for Linux personality safely. | Essential for OTA-oriented Android-style deployment. | **P2** |
| **Advanced accelerator governance** | **Services/device + runtime/lib** | Useful later for GPU/AI translation, not foundational today. | Important for future Android AI/media workloads, not first priority. | **P3** |
| **AI-informed scheduling policy** | **Service, not kernel** | Linux can benefit later, but kernel should not depend on opaque ML policy. | Android may use it later for tuning, but it must stay optional. | **P3** |
| **Carbon-aware scheduling** | **External policy/service/orchestrator** | Nice for cloud, irrelevant to core kernel correctness. | Not core for Android device OS. | **P3** |

## What already aligns

- Kernel is mechanism only.
- Services own policy and orchestration.
- No shared mutable state across cores.
- All cross-core actions should be message-based.
- External contracts belong in UAPI/core/personalities/stacks, not smeared into kernel.

## Production-grade top 10 shortlist

1. Per-core ownership.
2. Message-only cross-core actions.
3. Bounded invalidation protocol.
4. Per-core PMM caches.
5. Unified VM authority path.
6. Capability lifecycle.
7. Strict IPC authorization.
8. Memory class tags.
9. Timer/deadline primitives.
10. Fault domains + service supervision.

## One mistake to avoid

Do **not** add Linux or Android features directly into the kernel.

Prefer this layering discipline:

- Add **translation-grade kernel primitives**.
- Expose them through **stable UAPI**.
- Implement compatibility in **personalities**.
- Keep routing/power/AI/update policy in **services**.
- Keep composed domains like network/media/storage in **stacks**.

## Recommended execution order

### P0 now

- Per-core ownership.
- Bounded invalidation.
- Capability lifecycle and enforcement.
- PMM per-core caches.
- Invariant tests.

### P1 next

- Unified VM authority path.
- Fault domains.
- Timer/deadline classes.
- Power/thermal hooks.
- Service supervisor.

### P2 after that

- Device class model.
- Telemetry contract.
- Netif/routing substrate.
- Persistent event log.
- Secure update framework.

This sequence keeps Bharat-OS from becoming a Linux clone while making it a stronger host for Linux and Android translation.
