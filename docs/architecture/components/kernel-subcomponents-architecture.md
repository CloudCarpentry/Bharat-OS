# Kernel Subcomponents Architecture (Status + Roadmap Mapping)

This document decomposes kernel subcomponents, architecture coverage, implementation status, and roadmap alignment.

## Themed Mermaid view

```mermaid
%%{init: {'theme':'base','themeVariables':{'primaryColor':'#1d3557','primaryTextColor':'#ffffff','primaryBorderColor':'#457b9d','lineColor':'#a8dadc','secondaryColor':'#2a9d8f','tertiaryColor':'#264653','fontFamily':'Inter'}}}%%
graph TD
    K[Kernel Core] --> MM[Memory Management]
    K --> IPC[IPC + URPC]
    K --> CAP[Capability System]
    K --> SCH[Scheduler]
    K --> HAL[HAL Abstraction]

    HAL --> X86[x86_64]
    HAL --> ARM[arm64]
    HAL --> RV[riscv64]

    classDef done fill:#2a9d8f,stroke:#1f7a68,color:#fff;
    classDef partial fill:#e9c46a,stroke:#b5892e,color:#000;
    classDef todo fill:#e76f51,stroke:#b24d35,color:#fff;

    MM:::partial
    IPC:::partial
    CAP:::partial
    SCH:::partial
    HAL:::partial
```

## Themed PlantUML view

```plantuml
@startuml
!theme amiga
skinparam backgroundColor #0f172a
skinparam ArrowColor #93c5fd
skinparam defaultTextAlignment center
skinparam packageStyle rectangle

package "Kernel Core" {
  [Memory Mgmt]
  [IPC/URPC]
  [Capability System]
  [Scheduler]
  [HAL]
}

[HAL] --> [x86_64 path]
[HAL] --> [arm64 path]
[HAL] --> [riscv64 path]

[Kernel Core] -[hidden]- [HAL]
@enduml
```

## Subcomponent status and architecture detail

| Subcomponent | x86_64 | arm64 | riscv64 | Current status | What is done | What is next | Roadmap linkage |
| --- | --- | --- | --- | --- | --- | --- | --- |
| Memory management (PMM/VMM + MMU ops) | Present | Present | Present | Partial | Baseline page-table path and MMU interfaces exist. | TLB shootdown ack semantics, huge-page lifecycle, heap hardening. | Phase 1, Phase 3 |
| IPC/URPC | Present | Present | Present | Partial | Endpoint + URPC primitives in baseline direction. | Backpressure/failure handling, cross-node transport. | Phase 1, Phase 3 |
| Capability core | Present | Present | Present | Partial | Grant/delegate/revoke model direction in place. | Temporal/expiry controls, stronger policy proofs/audits. | Phase 1, Phase 4 |
| Scheduler + RT hooks | Present | Present | Present | Partial | Per-core scheduling direction and policy hooks. | Admission control depth (EDF/RMS), bounded AI influence. | Phase 1, Phase 2 |
| HAL contracts | Maturest | Active | Active | Partial | Multi-arch abstraction and bring-up paths exist. | Arch parity closure, validation depth, stronger platform tests. | Phase 1, Phase 3 |

## Implementation rule

- Keep architecture docs forward-looking.
- Keep implementation claims conservative and synchronized with `docs/current-code-status.md` and `ROADMAP.md`.
