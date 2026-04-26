---
title: Kernel Subcomponents Architecture (Repository-Aligned Status + Roadmap)
status: Proposed
owner: Documentation Working Group
last_updated: 2026-04-25
tags:
  - docs
  - architecture
  - components
see_also:
  - README.md
---
# Kernel Subcomponents Architecture (Repository-Aligned Status + Roadmap)

This document decomposes kernel subcomponents according to the **current `core/kernel/src` tree** and maps closure tasks.

## Repository-aligned kernel map

```mermaid
%%{init: {'theme':'base','themeVariables':{'primaryColor':'#1d3557','primaryTextColor':'#ffffff','lineColor':'#a8dadc','fontFamily':'Inter'}}}%%
graph TD
    K[core/kernel/src] --> CORE[core]
    K --> MM[mm]
    K --> IPC[ipc + urpc]
    K --> CAP[cap]
    K --> SCHED[sched]
    K --> TRAP[trap + sys]
    K --> SUBSYS[subsystem]
    K --> DEVICE[device + display + fs + console]
    K --> PROFILE[profile/*]

    MM --> PMM[pmm]
    MM --> PT[pt]
    MM --> TLB[tlb]
    MM --> IOMMU[iommu]
    MM --> DMA[dma]

    SUBSYS --> LINUX[linux]
```

## Alignment with `folder_structure.md`

| Target kernel intent | Current paths present | Alignment | Notes |
| --- | --- | --- | --- |
| Minimal mechanism-focused core | `core`, `sched`, `mm`, `ipc`, `cap`, `trap`, `sys` | Strong | Core mechanisms are clearly represented. |
| Keep policy out of kernel | `core/kernel/src/profile/*`, `core/kernel/src/subsystem/linux` | Partial | Some profile/personality-like concerns still reside under kernel tree. |
| Capability + IPC primitives | `cap`, `ipc`, `urpc` | Strong | Matches architecture direction. |
| Memory authority and isolation | `mm/{vm,pt,pmm,dma,iommu,tlb}` | Strong | Good decomposition for MM evolution. |
| Hardware abstraction usage | via `core/hal/*` and arch paths | Baseline | Arch-specific `ops` registration is now pushed into arch directories, keeping `core/hal` clean of `#ifdef` leakage. |

## Kernel status matrix

| Subcomponent | Current status | Evidence in tree | Next structural action | Roadmap linkage |
| --- | --- | --- | --- | --- |
| Memory management | Partial | `core/kernel/src/mm/*` broad coverage | Complete huge-page lifecycle, TLB shootdown protocol proof tests, IOMMU authority boundaries. | Phase 1, Phase 3 |
| IPC + URPC | Partial | `core/kernel/src/ipc`, `core/kernel/src/urpc` | Unify backpressure/error contracts and cross-node routing semantics. | Phase 1, Phase 3 |
| Capability core | Partial | `core/kernel/src/cap` | Add formal derivation/revocation invariant checks in host tests. | Phase 1, Phase 4 |
| Scheduler | Partial | `core/kernel/src/sched` | Improve admission control + deterministic RT isolation proofs. | Phase 1, Phase 2 |
| Trap/syscall path | Baseline | `core/kernel/src/trap`, `core/kernel/src/sys` | Unified syscall detection via `hal_cpu_is_syscall` contract. Hardened boot contract validation and PT alignment checks. | Phase 1 |
| Subsystem/personality hooks | Partial | `core/kernel/src/subsystem/linux`, `core/kernel/src/profile/*` | Move policy-heavy profile logic toward `core/services/` or `core/personalities/` where possible. | Phase 2, Phase 4 |

## Coding tasks identified

1. **Kernel profile extraction audit:** evaluate each `core/kernel/src/profile/*` module for migration to `core/services/` (policy) vs retention in kernel (mechanism).
2. **Subsystem boundary cleanup:** define strict interfaces between `core/kernel/src/subsystem/linux` and `core/personalities/compat/linux` to avoid duplication.
3. **Capability correctness tests:** expand host-side invariant suites for grant/delegate/revoke edge cases.
4. **MM + IOMMU contract tests:** add integration tests validating DMA map/unmap, revoke ordering, and TLB synchronization.
