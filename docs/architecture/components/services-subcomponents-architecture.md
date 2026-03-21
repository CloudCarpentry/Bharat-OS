# Services Subcomponents Architecture (Status + Roadmap Mapping)

This document maps service domains to their current implementation maturity and roadmap closure path.

## Mermaid (service domain map)

```mermaid
%%{init: {'theme':'base','themeVariables':{'primaryColor':'#14213d','primaryTextColor':'#ffffff','lineColor':'#fca311','fontFamily':'Inter'}}}%%
graph TB
    SV[Service Layer] --> CORE[Core Managers]
    SV --> NET[Network Services]
    SV --> PLATFORM[Platform Services]

    CORE --> MEM[memmgr]
    CORE --> SCH[schedmgr]
    CORE --> DEV[devmgr]
    CORE --> TEL[telemetrymgr]

    NET --> NM[netmgr]
    NET --> NS[netstack]
    NET --> NF[netfast]

    PLATFORM --> FS[file_system]
    PLATFORM --> CR[crypto]
    PLATFORM --> CON[console]
    PLATFORM --> BD[boot_displayd]

    classDef scaffold fill:#e63946,stroke:#9f1239,color:#fff;
    classDef partial fill:#ffb703,stroke:#b45309,color:#111;

    MEM:::scaffold
    SCH:::scaffold
    DEV:::scaffold
    TEL:::scaffold
    NM:::partial
    NS:::partial
    NF:::scaffold
    FS:::partial
    CR:::partial
    CON:::scaffold
    BD:::partial
```

## PlantUML (service groups)

```plantuml
@startuml
!theme spacelab
skinparam backgroundColor #0b132b
skinparam componentStyle rectangle
skinparam ArrowColor #5bc0be

package "Service Layer" {
  package "Core Managers" {
    [memmgr]
    [schedmgr]
    [devmgr]
    [telemetrymgr]
  }
  package "Network" {
    [netmgr]
    [netstack]
    [netfast]
  }
  package "Platform" {
    [file_system]
    [crypto]
    [console]
    [boot_displayd]
  }
}

[netmgr] --> [netstack]
[file_system] --> [drivers]
@enduml
```

## Service status matrix

| Service area | Current status | Done | To do | Roadmap linkage |
| --- | --- | --- | --- | --- |
| Core managers (`coremgr`, `memmgr`, `schedmgr`, `devmgr`, `storagemgr`, `faultmgr`, `telemetrymgr`) | Scaffold-heavy | Build wiring and service boundaries exist. | Implement stable event loops, IPC contracts, policy engines, and lifecycle integration. | Phase 1, Phase 2 |
| Naming/orchestration (`init`, `namesvc`, `servicemgr`) | Scaffold/Partial | Bootstrap and registry basics exist. | Production-ready capability-safe registration/discovery and orchestration control flow. | Phase 1 |
| Network (`netmgr`, `netstack`, `netfast`, legacy `net`) | Partial | Control-plane tables and protocol modules exist for net split. | Security enforcement, fast-path integration, daemon loop hardening, TCP/depth closure. | Phase 1, Phase 3 |
| Platform (`file_system`, `crypto`, `console`, `boot_displayd`) | Partial/Scaffold | VFS and crypto service skeleton plus boot display baseline exist. | Durable storage backend, full IPC transport, richer console/UX/runtime integration. | Phase 2, Phase 3 |
