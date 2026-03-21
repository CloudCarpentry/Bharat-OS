# Subsystem Subcomponents Architecture (Status + Roadmap Mapping)

This document tracks subsystem-level decomposition, status, and roadmap alignment.

## Mermaid (subsystem view)

```mermaid
%%{init: {'theme':'base','themeVariables':{'primaryColor':'#003049','primaryTextColor':'#ffffff','lineColor':'#fcbf49','fontFamily':'Inter'}}}%%
graph LR
    S[Subsystem Layer] --> LNX[Linux Personality]
    S --> AND[Android Personality]
    S --> WIN[Windows Compat]
    S --> AUTO[Automotive]
    S --> NETC[Network Contracts]
    S --> SKB[System Knowledge Base]

    classDef partial fill:#fcbf49,stroke:#d68c00,color:#111;
    classDef scaffold fill:#d62828,stroke:#9b1c1c,color:#fff;

    LNX:::partial
    AND:::partial
    WIN:::partial
    AUTO:::partial
    NETC:::partial
    SKB:::partial
```

## PlantUML (subsystem packaging)

```plantuml
@startuml
!theme plain
skinparam backgroundColor #111827
skinparam packageBackgroundColor #1f2937
skinparam packageBorderColor #60a5fa
skinparam defaultTextAlignment center

package "Subsystem Layer" {
  [Linux Personality]
  [Android Personality]
  [Windows Compatibility]
  [Automotive Module]
  [Network Contracts]
  [SKB]
}

[Linux Personality] --> [Network Contracts]
[Android Personality] --> [Network Contracts]
@enduml
```

## Subsystem status matrix

| Subcomponent | Scope | Current status | Done | To do | Roadmap linkage |
| --- | --- | --- | --- | --- | --- |
| Linux personality layer | Syscall/personality adaptation | Partial | Contract and compatibility scaffolding exists. | Syscall translation completeness and runtime conformance tests. | Phase 4 |
| Android personality layer | Android compatibility domain | Partial | Core module and architecture docs exist. | Binder/ashmem/runtime coverage expansion. | Phase 4 |
| Windows compatibility shims | API compatibility helpers | Partial | Compatibility shim structure is present. | Broader API depth and behavioral tests. | Phase 4 |
| Automotive subsystem | RT/automotive profile hooks | Partial | Module and profile intent exists. | Deterministic fault containment and RT validation suites. | Phase 2 |
| Network contracts (`subsys/network`) | Shared control/data contracts | Partial | Types/uAPI/contract headers available. | Runtime enforcement and policy integration depth. | Phase 1, Phase 3 |
| SKB and topology surfaces | Platform topology and routing hints | Partial | Base SKB direction exists in subsystem layer. | Better topology ingestion and runtime policy feedback loop. | Phase 1 |
